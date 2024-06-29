#include "frame.h"

#include <functional>
#include <stdexcept>

#include "error.h"

namespace http2 {

std::span<const uint8_t> header_frame::header_block() const {
  const uint8_t *begin = reinterpret_cast<const uint8_t *>(&length) + sizeof(header);
  std::size_t offset = 0;
  std::size_t pad_length = 0;
  if (flags & flags::PADDED) {
    ++offset;
    pad_length = *begin;
  }
  if (flags & flags::PRIORITY) {
    offset += 5;
  }

  return {begin + offset, payload_size() - offset - pad_length};
}

std::span<const uint8_t> data_frame::data() const {
  const uint8_t *begin = reinterpret_cast<const uint8_t *>(&length) + sizeof(header);
  std::size_t offset = 0;
  std::size_t pad_length = 0;
  if (flags & flags::PADDED) {
    ++offset;
    pad_length = *begin;
  }

  return {begin + offset, payload_size() - offset - pad_length};
}

std::span<const uint8_t> continuation_frame::header_block() const {
  const uint8_t *begin = reinterpret_cast<const uint8_t *>(&length) + sizeof(header);
  return {begin, payload_size()};
}

frame_analyzer::frame_analyzer(std::span<const uint8_t> b) : buffer(b) {
  // adjust size
  buffer = buffer.subspan(0, std::min(size(), buffer.size_bytes()));
}

frame_analyzer frame_analyzer::from_buffer(std::span<const uint8_t> buffer) {
  // Check minimal frame size
  if (buffer.size_bytes() < frame_analyzer::min_size()) {
    throw std::length_error("not enought data for a valid frame");
  }

  return frame_analyzer(buffer);
}

void frame_analyzer::check(std::size_t max_frame_size) const {
  auto &header = frame_header();
  if (static_cast<std::size_t>(header.type) > static_cast<std::size_t>(frame_type::CONTINUATION)) {
    throw boost::system::system_error(make_error_code(error_code::PROTOCOL_ERROR), "Unknown frame type");
  }

  if (header.payload_size() > max_frame_size) {
    throw boost::system::system_error(make_error_code(error_code::FRAME_SIZE_ERROR), "Overflow payload size");
  }

  auto frame_type = static_cast<int>(header.type);
  std::invoke(check_table[frame_type], this);
}

void frame_analyzer::check_data_frame() const {}
void frame_analyzer::check_headers_frame() const {}
void frame_analyzer::check_priority_frame() const {}
void frame_analyzer::check_reset_frame() const {}
void frame_analyzer::check_settings_frame() const {}
void frame_analyzer::check_push_frame() const {}
void frame_analyzer::check_ping_frame() const {}
void frame_analyzer::check_goaway_frame() const {}
void frame_analyzer::check_window_update_frame() const {}
void frame_analyzer::check_continuation_frame() const {}

frame_analyzer::check_ptr frame_analyzer::check_table[] = {
    &frame_analyzer::check_data_frame,          &frame_analyzer::check_headers_frame,
    &frame_analyzer::check_priority_frame,      &frame_analyzer::check_reset_frame,
    &frame_analyzer::check_settings_frame,      &frame_analyzer::check_push_frame,
    &frame_analyzer::check_ping_frame,          &frame_analyzer::check_goaway_frame,
    &frame_analyzer::check_window_update_frame, &frame_analyzer::check_continuation_frame,
};

} // namespace http2
