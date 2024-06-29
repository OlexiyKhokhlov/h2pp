#include "base_client.h"

#include <functional>

#include "hpack/decoder.h"
#include "hpack/encoder.h"

#include "dummy_window.h"
#include "error.h"
#include "frame_builder.h"
#include "settings_manager.h"
#include "stream_registry.h"

namespace http2 {

decltype(&base_client::on_receive_headers) base_client::frame_handlers[] = {
    &base_client::on_receive_data_not_reachable,
    &base_client::on_receive_headers,
    &base_client::on_receive_priority,
    &base_client::on_receive_reset,
    &base_client::on_receive_settings,
    &base_client::on_receive_push,
    &base_client::on_receive_ping,
    &base_client::on_receive_goaway,
    &base_client::on_receive_window_update,
    &base_client::on_receive_continuation,
};

struct base_client::PrivateClient {
  explicit PrivateClient(boost::asio::io_context &io)
      : settings(io), local_window(http2::INITIAL_WINDOW_SIZE, http2::INITIAL_WINDOW_SIZE / 4) {}

  // HPACK
  rfc7541::decoder decoder;
  rfc7541::encoder encoder;
  // Settings
  settings_manager settings;
  // Stream registry
  stream_registry registry;
  dummy_window local_window;

  template <typename F, typename... Args> bool invoke_for_stream(uint32_t stream_id, F method, Args... args) {
    stream::ptr stream_ptr = registry.get_stream(stream_id);
    if (!stream_ptr) {
      return false;
    }

    std::invoke(method, stream_ptr, std::forward<Args>(args)...);

    if (stream_ptr->is_finished()) {
      registry.erase(stream_id);
    } else {
      if (stream_ptr->check_tx_data()) {
        registry.enqueue(stream_ptr);
      }
    }
    return true;
  }
};

base_client::base_client(boost::asio::io_context &io)
    : io(io), private_client(new PrivateClient(io)), server_window_size{http2::INITIAL_WINDOW_SIZE} {}

base_client::~base_client() = default;

utils::buffer base_client::on_read(utils::buffer &&io_buff) {
  auto incomming_bytes = io_buff.data_view();
  // TODO: Probably it will be good to check frame length and frame type as early as possible
  //       even when a frame header is not complete.
  if (incomming_bytes.size_bytes() < frame_analyzer::min_size()) {
    return io_buff;
  }

  std::size_t used_bytes = 0;
  try {
    while (incomming_bytes.size_bytes() >= frame_analyzer::min_size()) {

      auto frame = frame_analyzer::from_buffer(incomming_bytes);
      frame.check(private_client->settings.get_local_settings().max_frame_size);

      if (frame.is_complete()) {
        const auto &header = frame.frame_header();
        if (header.type == frame_type::DATA) {
          // Optimization. DATA frames will be moved and stored into 'stream' class
          if (frame.raw_bytes().data() == io_buff.data_view().data() &&
              frame.raw_bytes().size_bytes() == io_buff.data_view().size_bytes()) {
            on_receive_data_frame(std::move(io_buff));
            incomming_bytes = {};
            break;
          }
          auto frame_buff = utils::buffer(frame.raw_bytes().size_bytes());
          frame_buff.commit(frame.raw_bytes());
          on_receive_data_frame(std::move(frame_buff));
        } else {
          // All other frames are processed on the fly via 'span'.
          on_receive_frame(frame.raw_bytes());
        }

        // To the next frame in the io_buffer
        incomming_bytes = incomming_bytes.subspan(frame.raw_bytes().size_bytes());
        used_bytes += frame.raw_bytes().size_bytes();

      } else {
        // a frame is not complete
        if (io_buff.data_view().size_bytes() + io_buff.prepare().size_bytes() < frame.size()) {
          auto big_buff = utils::buffer(frame.size());
          big_buff.commit(frame.raw_bytes());
          used_bytes = 0;
          io_buff = std::move(big_buff);
        }
        break;
      }
    }
  } catch (const boost::system::system_error &ex) {
    initiate_disconnect(ex.code());
  } catch (const std::exception & /*ex*/) {
    initiate_disconnect(error_code::INTERNAL_ERROR);
  } catch (...) {
    initiate_disconnect(error_code::INTERNAL_ERROR);
  }

  init_write();

  if (incomming_bytes.empty()) {
    return utils::buffer(64);
  }

  if (used_bytes != 0) {
    io_buff.consume(used_bytes);
  }

  return io_buff;
}

void base_client::on_receive_frame(std::span<const uint8_t> data) {

  private_client->local_window.dec(data.size_bytes());
  auto upd_window = private_client->local_window.update(0);
  if (upd_window) {
    send_command(std::move(upd_window.value()));
  }

  auto fanalyzer = frame_analyzer::from_buffer(data);
  auto frame_type = static_cast<int>(fanalyzer.frame_header().type);
  std::invoke(frame_handlers[frame_type], this, data);
}

void base_client::on_receive_data_frame(utils::buffer &&buff) {
  private_client->local_window.dec(buff.data_view().size_bytes());
  auto upd_window = private_client->local_window.update(0);
  if (upd_window) {
    send_command(std::move(upd_window.value()));
  }

  on_receive_data(std::move(buff));
}

std::deque<utils::buffer> base_client::get_tx_data() {

  // 1. Move command frames
  decltype(tx_command_queue) result;
  {
    std::scoped_lock lock(command_queue_mutex);
    while (!tx_command_queue.empty()) {
      const auto data = tx_command_queue.front().data_view();
      if (data.size_bytes() > server_window_size) {
        return result;
      }
      server_window_size -= data.size_bytes();
      result.emplace_back(std::move(tx_command_queue.front()));
      tx_command_queue.pop_front();
    }
  }

  // 2. Move stream frames
  auto limit = std::min(std::size_t(server_window_size),
                        std::size_t(private_client->settings.get_local_settings().max_frame_size));
  if (limit >= sizeof(header) * 2) {
    server_window_size -= private_client->registry.get_data(result, private_client->encoder, limit);
  }

  return result;
}

void base_client::cleanup_after_disconnect(const boost::system::error_code &ec) {
  private_client->registry.reset(ec);
  start_connecting_flag.clear();
}

void base_client::send_command(utils::buffer &&buff) {
  std::scoped_lock lock(command_queue_mutex);
  tx_command_queue.emplace_back(std::move(buff));
}

void base_client::write_initial_frames() {
  static char preambula[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

  utils::buffer buff(sizeof(preambula) - 1);
  buff.commit({reinterpret_cast<const uint8_t *>(&preambula[0]), sizeof(preambula) - 1});

  send_command(std::move(buff));
}

void base_client::initiate_sync_settings(
    bool remote_sync, boost::asio::any_completion_handler<void(boost::system::error_code)> &&handler) {
  http2::settings default_settings;

  auto h = [last = std::move(handler), this](const boost::system::error_code &ec) mutable {
    if (!ec) {
      // Adjust per session local window size after successfull changing local settings
      auto window_size = private_client->settings.get_local_settings().initial_window_size *
                         private_client->settings.get_local_settings().max_concurrent_streams;

      auto window_increment = window_size - private_client->local_window.current();
      private_client->local_window = dummy_window(window_size, window_size / 2);

      send_command(frame_builder::update_window(window_increment, 0));
      init_write();
    }
    last(ec);
  };

  auto buff = private_client->settings.initiate_sync_settings(std::move(default_settings), remote_sync, std::move(h));
  send_command(std::move(buff));
  init_write();
}

void base_client::initiate_disconnect(boost::system::error_code ec,
                                      boost::asio::any_completion_handler<void()> &&handler) {
  if (start_disconnect_flag.test_and_set()) {
    if (handler) {
      handler();
    }
    return;
  }

  if (private_client->settings.cancel(ec)) {
    return;
  }

  connection_error_code = ec;
  disconnect_handler = std::move(handler);

  // FIXME: Send correct stream id
  send_command(frame_builder::goaway(error_code::IS_OK, 1));
  init_write();
}

void base_client::initiate_ping(boost::asio::any_completion_handler<void(boost::system::error_code)> &&handler) {
  if (!is_connected_flag.test()) {
    throw boost::system::system_error(boost::asio::error::not_connected, "Client is not connected");
  }
  if (ping_handler) {
    throw boost::system::system_error(boost::asio::error::in_progress, "Client is not connected");
  }

  ping_handler = std::move(handler);
  send_command(frame_builder::ping({}));
}

void base_client::initiate_send(
    request &&rq, boost::asio::any_completion_handler<void(boost::system::error_code, response &&)> &&handler) {
  if (!is_connected_flag.test()) {
    throw boost::system::system_error(boost::asio::error::not_connected, "Client is not connected");
  }

  auto id = get_next_stream_id();
  stream::ptr sptr(new stream(io, id, private_client->settings.get_server_settings().initial_window_size,
                              private_client->settings.get_local_settings().initial_window_size, std::move(rq),
                              std::move(handler)));

  private_client->registry.add_stream(id, sptr);
  init_write();
}

void base_client::on_receive_data(utils::buffer &&buff) {
  const auto analyzer = frame_analyzer::from_buffer(buff.data_view());
  const auto &frame = analyzer.get_frame<frame_type::DATA>();

  /*auto processed =*/private_client->invoke_for_stream(frame.stream_id, &stream::on_receive_data, std::move(buff));
}

void base_client::on_receive_data_not_reachable(std::span<const uint8_t>) {
  // All frame handler accept an input data as std::span<const uint8_t>
  // And only DATA frame by optimization reason accepts it via buffer moving.
  throw boost::system::system_error(make_error_code(error_code::INTERNAL_ERROR), "Unreachable code");
}

void base_client::on_receive_headers(std::span<const uint8_t> data) {
  const auto analyzer = frame_analyzer::from_buffer(data);
  const auto &headers_frame = analyzer.get_frame<frame_type::HEADERS>();

  rfc7541::header fields;
  try {
    fields = private_client->decoder.decode(headers_frame.header_block());
  } catch (const std ::exception &ex) {
    throw boost::system::system_error(make_error_code(error_code::COMPRESSION_ERROR), ex.what());
  }
  /*auto processed =*/private_client->invoke_for_stream(headers_frame.stream_id, &stream::on_receive_headers,
                                                        std::move(fields), headers_frame.flags, data.size_bytes());
}

void base_client::on_receive_priority(std::span<const uint8_t>) {}

void base_client::on_receive_reset(std::span<const uint8_t> data) {
  const auto analyzer = frame_analyzer::from_buffer(data);
  const auto &rst_frame = analyzer.get_frame<frame_type::RST_STREAM>();

  /*auto processed =*/private_client->invoke_for_stream(rst_frame.stream_id, &stream::on_receive_reset, rst_frame.code);
}

void base_client::on_receive_settings(std::span<const uint8_t> data) {
  auto opt_buff = private_client->settings.on_settings_frame(data);
  if (opt_buff) {
    send_command(std::move(opt_buff.value()));
  }
}

void base_client::on_receive_push(std::span<const uint8_t>) {
  send_command(frame_builder::goaway(error_code::PROTOCOL_ERROR, 1));
  init_write();
}

void base_client::on_receive_ping(std::span<const uint8_t> data) {
  const auto analyzer = frame_analyzer::from_buffer(data);
  const ping_frame &ping = analyzer.get_frame<frame_type::PING>();
  if ((ping.flags & flags::ACK) != 0) {
    if (ping_handler) {
      decltype(ping_handler) h;
      h.swap(ping_handler);
      h(boost::system::error_code{});
    }
  } else {
    utils::buffer ack_buffer(data.size_bytes());
    ack_buffer.commit(data);
    auto ack_analyzer = frame_analyzer::from_buffer(ack_buffer.data_view());
    auto &ack_header = ack_analyzer.frame_header();
    const_cast<header &>(ack_header).flags |= flags::ACK;
    send_command(std::move(ack_buffer));
  }
}

void base_client::on_receive_goaway(std::span<const uint8_t> /*data*/) {
  //  const auto analyzer = frame_analyzer::from_buffer(data);
  //  const auto &frame = analyzer.get_frame<frame_type::GOAWAY>();
  //  auto additional = frame.additional();
}

void base_client::on_receive_window_update(std::span<const uint8_t> data) {
  const auto analyzer = frame_analyzer::from_buffer(data);
  const auto &frame = analyzer.get_frame<frame_type::WINDOW_UPDATE>();
  auto size_increment = frame.window_size;
  if (frame.stream_id == 0) {
    server_window_size += size_increment;
  } else {
    /*auto processed =*/private_client->invoke_for_stream(frame.stream_id, &stream::on_receive_window_update,
                                                          size_increment);
  }
}

void base_client::on_receive_continuation(std::span<const uint8_t> data) {
  const auto analyzer = frame_analyzer::from_buffer(data);
  const auto &continuation_frame = analyzer.get_frame<frame_type::CONTINUATION>();

  rfc7541::header fields;
  try {
    fields = private_client->decoder.decode(continuation_frame.header_block());
  } catch (const std ::exception &ex) {
    throw boost::system::system_error(make_error_code(error_code::COMPRESSION_ERROR), ex.what());
  }

  /*auto processed =*/private_client->invoke_for_stream(continuation_frame.stream_id, &stream::on_receive_continuation,
                                                        std::move(fields), continuation_frame.flags, data.size_bytes());
}

} // namespace http2
