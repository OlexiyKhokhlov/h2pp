#include "frame.h"

#include <functional>
#include <stdexcept>

#include "error.h"

namespace http2 {

std::span<const uint8_t> header_frame::header_block() const {
  auto payload_span = payload();
  if (flags & flags::PADDED) {
    payload_span = payload_span.subspan(1, payload_span.size_bytes() - 1 - payload_span[0]);
  }
  if (flags & flags::PRIORITY) {
    payload_span = payload_span.subspan(5);
  }
  return payload_span;
}

std::span<const uint8_t> data_frame::data() const {
  auto payload_span = payload();
  if (flags & flags::PADDED) {
    payload_span = payload_span.subspan(1, payload_span.size_bytes() - 1 - payload_span[0]);
  }
  return payload_span;
}

std::span<const uint8_t> continuation_frame::header_block() const { return payload(); }

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
    throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Unknown frame type");
  }

  if (header.payload_size() > max_frame_size) {
    throw boost::system::system_error(error_code::FRAME_SIZE_ERROR, "Overflow payload size");
  }

  auto frame_type = static_cast<int>(header.type);
  std::invoke(check_table[frame_type], this);
}

void frame_analyzer::check_data_frame() const {
  const data_frame &frame = *reinterpret_cast<const struct data_frame *>(buffer.data());
  if (frame.flags & ~flags::DATA_ALLOWED_FLAGS_MASK) {
    throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Invalid flag");
  }
  if (frame.stream_id & 0x80000000 || frame.stream_id == 0) {
    throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Invalid stream ID");
  }
  // if (frame.flags & flags::PADDED && frame.padding() ==0){
  //   throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Padding cannot be 0");
  // }
}

void frame_analyzer::check_headers_frame() const {
  const header_frame &frame = *reinterpret_cast<const struct header_frame *>(buffer.data());
  if (frame.flags & ~flags::HEADERS_ALLOWED_FLAGS_MASK) {
    throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Invalid flag");
  }
  if (frame.stream_id & 0x80000000 || frame.stream_id == 0) {
    throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Invalid stream ID");
  }
  if (frame.payload_size() == 0 && frame.flags & flags::END_HEADERS) {
    throw boost::system::system_error(error_code::FRAME_SIZE_ERROR, "Unexpected empty frame payload");
  }
}

void frame_analyzer::check_priority_frame() const {}

void frame_analyzer::check_reset_frame() const {}

void frame_analyzer::check_settings_frame() const {
  const settings_frame &frame = *reinterpret_cast<const struct settings_frame *>(buffer.data());
  if (frame.flags & ~flags::SETTINGS_ALLOWED_FLAGS_MASK) {
    throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Invalid flag");
  }
  if (frame.stream_id != 0) {
    throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Invalid stream id");
  }
  if (frame.payload_size() % sizeof(settings_item) != 0) {
    throw boost::system::system_error(error_code::FRAME_SIZE_ERROR, "Invalid settings size");
  }
}

void frame_analyzer::check_push_frame() const {
  throw boost::system::system_error(error_code::INTERNAL_ERROR, "PUSH is not supported");
}

void frame_analyzer::check_ping_frame() const {
  const ping_frame &frame = *reinterpret_cast<const struct ping_frame *>(buffer.data());
  if (frame.flags & ~flags::PING_ALLOWED_FLAGS_MASK) {
    throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Invalid flag");
  }
  if (frame.stream_id != 0) {
    throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Invalid stream id");
  }
  if (frame.payload_size() > 64) {
    throw boost::system::system_error(error_code::FRAME_SIZE_ERROR, "Invalid payload size");
  }
}

void frame_analyzer::check_goaway_frame() const {}

void frame_analyzer::check_window_update_frame() const {
  const continuation_frame &frame = *reinterpret_cast<const struct continuation_frame *>(buffer.data());
  if (frame.flags != 0) {
    throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Invalid flag");
  }
  if (frame.stream_id & 0x80000000) {
    throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Invalid stream ID");
  }
}

void frame_analyzer::check_continuation_frame() const {
  const continuation_frame &frame = *reinterpret_cast<const struct continuation_frame *>(buffer.data());
  if (frame.flags & ~flags::CONTINUATION_ALLOWED_FLAGS_MASK) {
    throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Invalid flag");
  }
  if (frame.stream_id & 0x80000000 || frame.stream_id == 0) {
    throw boost::system::system_error(error_code::PROTOCOL_ERROR, "Invalid stream ID");
  }
  if (frame.payload_size() == 0 && frame.flags & flags::END_HEADERS) {
    throw boost::system::system_error(error_code::FRAME_SIZE_ERROR, "Unexpected empty frame payload");
  }
}

frame_analyzer::check_ptr frame_analyzer::check_table[] = {
    &frame_analyzer::check_data_frame,          &frame_analyzer::check_headers_frame,
    &frame_analyzer::check_priority_frame,      &frame_analyzer::check_reset_frame,
    &frame_analyzer::check_settings_frame,      &frame_analyzer::check_push_frame,
    &frame_analyzer::check_ping_frame,          &frame_analyzer::check_goaway_frame,
    &frame_analyzer::check_window_update_frame, &frame_analyzer::check_continuation_frame,
};

} // namespace http2
