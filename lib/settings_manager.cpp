#include "settings_manager.h"

#include <chrono>

#include "error.h"
#include "frame.h"
#include "frame_builder.h"

#include <boost/endian.hpp>

using namespace std::chrono_literals;

namespace {
constexpr auto SettingsTimeout = 5s;
}

namespace http2 {
settings_manager::settings_manager(boost::asio::io_context &context) : settings_timer(context) {}

settings_manager::~settings_manager() = default;

utils::buffer settings_manager::initiate_sync_settings(
    settings &&local, bool remote_sync,
    boost::asio::any_completion_handler<void(boost::system::error_code)> &&handler) {
  {
    std::scoped_lock lock(mutex);
    if (settings_handler) {
      throw boost::system::system_error(boost::asio::error::in_progress, "Sync settings in the progress");
    }
  }

  local_settings = local;
  settings_handler = std::move(handler);
  need_remote_sync = remote_sync;
  local_settings_ack = false;
  remote_settings_got = !need_remote_sync;

  settings_timer.expires_from_now(SettingsTimeout);
  settings_timer.async_wait([this](const auto &ec) {
    if (ec != boost::asio::error::operation_aborted) {
      finished(make_error_code(error_code::SETTINGS_TIMEOUT));
    }
  });

  return frame_builder::settings({
      {settings_type::HEADER_TABLE_SIZE, local_settings.header_table_size},
      {settings_type::ENABLE_PUSH, uint32_t(local_settings.enable_push)},
      {settings_type::MAX_CONCURRENT_STREAMS, local_settings.max_concurrent_streams},
      {settings_type::INITIAL_WINDOW_SIZE, local_settings.initial_window_size},
      {settings_type::MAX_FRAME_SIZE, local_settings.max_frame_size},
      {settings_type::MAX_HEADER_LIST_SIZE, local_settings.max_header_list_size},
  });
}

std::optional<utils::buffer> settings_manager::on_settings_frame(std::span<const uint8_t> data) {
  const auto analyzer = frame_analyzer::from_buffer(data);
  const auto &frame = analyzer.get_frame<frame_type::SETTINGS>();
  if (frame.flags & flags::ACK) {
    local_settings_ack = true;
  } else {
    for (const auto &i : frame.items()) {
      switch (i.type) {
      case settings_type::HEADER_TABLE_SIZE:
        remote_settings.header_table_size = boost::endian::big_to_native(i.value);
        break;
      case settings_type::ENABLE_PUSH:
        remote_settings.enable_push =
            boost::endian::big_to_native(i.value) == 0 ? push_state::DISABLED : push_state::ENABLED;
        break;
      case settings_type::MAX_CONCURRENT_STREAMS:
        remote_settings.max_concurrent_streams = boost::endian::big_to_native(i.value);
        break;
      case settings_type::INITIAL_WINDOW_SIZE:
        remote_settings.initial_window_size = boost::endian::big_to_native(i.value);
        break;
      case settings_type::MAX_FRAME_SIZE:
        remote_settings.max_frame_size = boost::endian::big_to_native(i.value);
        break;
      case settings_type::MAX_HEADER_LIST_SIZE:
        remote_settings.max_header_list_size = boost::endian::big_to_native(i.value);
        break;
      case settings_type::ENABLE_CONNECT_PROTOCOL:
        remote_settings.connection_protocol = boost::endian::big_to_native(i.value) == 0
                                                  ? connection_protocol_state::DISABLED
                                                  : connection_protocol_state::ENABLED;
        break;
      }
    }

    remote_settings_got = true;

    // Send ACK
    return frame_builder::settings_ack();
  }

  settings_timer.cancel();
  if ((need_remote_sync && remote_settings_got && local_settings_ack) || (!need_remote_sync && local_settings_ack)) {
    finished(boost::system::error_code{});
  }

  return std::nullopt;
}

bool settings_manager::cancel(const boost::system::error_code &ec) {
  settings_timer.cancel();
  return finished(ec);
}

bool settings_manager::finished(const boost::system::error_code &ec) {
  std::scoped_lock lock(mutex);

  if (settings_handler) {
    decltype(settings_handler) h;
    settings_handler.swap(h);
    h(ec);
    return true;
  }
  return false;
}

} // namespace http2
