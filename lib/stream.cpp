#include "stream.h"

#include <ranges>

#include <boost/url.hpp>

#include "error.h"
#include "frame_builder.h"
#include "hpack/encoder.h"

namespace http2 {
stream::stream(boost::asio::io_context &io, boost::endian::big_uint32_t id, std::size_t remote_size,
               std::size_t local_size, request &&r,
               boost::asio::any_completion_handler<void(boost::system::error_code, response &&)> &&handler)
    : timer(io), http_id(id), remote_window(remote_size), local_window(local_size, local_size / 4),
      m_request(std::move(r)), respone_handler(std::move(handler)) {}

stream::~stream() = default;

bool stream::has_tx_data() const {
  return http_state == HttpState::HALF_CLOSED || local_window.need_update() || !m_request.header_list.empty() ||
         !m_request.body_list.empty();
}

bool stream::check_tx_data() {
  if (!is_scheduled && has_tx_data()) {
    is_scheduled = true;
    return true;
  }
  return false;
}

void stream::reset(const boost::system::error_code &ec) {
  http_state = HttpState::CLOSED;
  timer.cancel();
  finished(ec);
}

void stream::on_receive_data(utils::buffer &&buff) {
  local_window.dec(buff.data_view().size_bytes());
  const auto analyzer = frame_analyzer::from_buffer(buff.data_view());
  const auto &header = analyzer.frame_header();

  if (header.payload_size() != 0) {
    m_response.insert_body(std::move(buff));
  }
  if (header.flags & flags::END_STREAM) {
    http_state = HttpState::HALF_CLOSED;
    timer.cancel();
    finished(boost::system::error_code{});
  }
}

void stream::on_receive_headers(rfc7541::header &&header, uint8_t flags, std::size_t raw_size) {
  local_window.dec(raw_size);
  m_response.insert_headers(std::move(header));

  if (flags & flags::END_STREAM) {
    http_state = HttpState::HALF_CLOSED;
    timer.cancel();
    finished(boost::system::error_code{});
  }
}

void stream::on_receive_reset(error_code err) {
  http_state = HttpState::CLOSED;
  finished(make_error_code(err));
}

void stream::on_receive_window_update(uint32_t increment) {
  remote_window += increment;
}

void stream::on_receive_continuation(rfc7541::header &&header, uint8_t flags, std::size_t raw_size) {
  local_window.dec(raw_size);
  m_response.insert_headers(std::move(header));

  if (flags & flags::END_STREAM) {
    finished(boost::system::error_code{});
  }
}

std::size_t stream::prepare_headers(std::deque<utils::buffer> &out, rfc7541::encoder &encoder, std::size_t limit) {
  std::size_t used = 0;
  auto &rq = get_request();
  auto &headers = rq.get_headers();

  auto [buffers, count] =
      encoder.encode(headers, is_cointinuation ? limit - 2 * sizeof(header) : limit - sizeof(header));
  if (count == 0) {
    return used;
  }

  rq.commit_headers(count);

  // TODO: have a size inside buffers
  std::size_t buffer_size = 0;
  for (const auto &b : buffers) {
    buffer_size += b.data_view().size_bytes();
  }

  uint8_t flags = headers.empty() ? flags::END_HEADERS : 0;

  if (is_cointinuation) {
    auto frame_header = frame_builder::continuation(id(), flags, buffer_size);
    used += frame_header.data_view().size_bytes();
    out.emplace_back(std::move(frame_header));
  } else {
    flags = rq.body_list.empty() ? flags | flags::END_STREAM : flags;
    auto frame_header = frame_builder::headers(id(), flags, buffer_size);
    used += frame_header.data_view().size_bytes();
    out.emplace_back(std::move(frame_header));
    is_cointinuation = !headers.empty();
    timer.expires_after(m_request.timeout());
    timer.async_wait([this](const auto &ec) {
      if (ec != boost::asio::error::operation_aborted) {
        finished(make_error_code(error_code::SETTINGS_TIMEOUT));
      }
    });
  }

  used += buffer_size;
  std::move(buffers.begin(), buffers.end(), std::back_inserter(out));

  if (is_cointinuation && headers.empty() && rq.body_list.empty()) {
    auto [frame_header, payload] = frame_builder::data(id(), flags::END_STREAM, 0);
    out.emplace_back(std::move(frame_header));
    used += frame_header.data_view().size_bytes();
  }

  return used;
}

std::size_t stream::prepare_body(std::deque<utils::buffer> &out, std::size_t limit) {
  auto left_size = m_request.body_size() - send_body_offset;
  auto payload_size = std::min(limit - sizeof(data_frame), left_size);

  bool is_last = payload_size == left_size;
  auto [frame_header, payload] = frame_builder::data(id(), is_last ? flags::END_STREAM : 0, payload_size);

  out.emplace_back(std::move(frame_header));

  auto body_it = m_request.body_list.begin();
  auto span_it = m_request.span_list.begin();
  do {
    auto to_copy = std::min(payload.size_bytes(), span_it->size_bytes() - send_body_offset);
    memcpy(payload.data(), span_it->data() + send_body_offset, to_copy);
    send_body_offset += to_copy;
    if (send_body_offset == span_it->size_bytes()) {
      send_body_offset = 0;
      body_it++;
      span_it++;
      m_request.body_list.pop_front();
      m_request.span_list.pop_front();
    }
    payload = payload.subspan(to_copy);
  } while (payload.size_bytes() != 0);

  return frame_header.data_view().size_bytes();
}

std::size_t stream::get_tx_data(std::deque<utils::buffer> &out, rfc7541::encoder &encoder, std::size_t limit) {
  is_scheduled = false;

  if (http_state == HttpState::HALF_CLOSED) {
    http_state = HttpState::CLOSED;
    auto last_frame = frame_builder::reset(error_code::IS_OK, http_id);
    auto used = last_frame.data_view().size_bytes();
    out.emplace_back(std::move(last_frame));
    return used;
  }

  limit = std::min(limit, remote_window);
  if (limit <= 16) {
    return 0;
  }

  http_state = HttpState::OPEN;

  std::size_t bytes_used = 0;
  auto upd_window = local_window.update(http_id);
  if (upd_window) {
    bytes_used += upd_window->data_view().size_bytes();
    out.push_back(std::move(upd_window.value()));
  }

  auto &rq = get_request();
  auto &headers = rq.get_headers();
  if (!headers.empty()) {
    bytes_used += prepare_headers(out, encoder, limit);
  }

  if (headers.empty() && rq.body_size() != 0 && (limit - bytes_used) >= 2 * sizeof(data_frame)) {
    bytes_used += prepare_body(out, limit - bytes_used);
  }
  remote_window -= bytes_used;
  return bytes_used;
}

void stream::finished(const boost::system::error_code &ec) {
  std::scoped_lock lock(mutex);

  if (respone_handler) {
    decltype(respone_handler) h;
    respone_handler.swap(h);
    h(ec, std::move(m_response));
  }
}

} // namespace http2
