#include "frame_builder.h"

namespace http2::frame_builder {

utils::buffer goaway(error_code err, boost::endian::big_uint32_t last_stream_id, std::span<const uint8_t> additional) {
  utils::buffer buffer(sizeof(goaway_frame) + additional.size());
  goaway_frame *frame = reinterpret_cast<goaway_frame *>(buffer.prepare().data());
  frame->type = frame_type::GOAWAY;
  frame->flags = 0;
  frame->stream_id = 0;
  frame->last_stream_id = last_stream_id;
  frame->error = err;
  frame->set_payload_size(sizeof(goaway_frame) - sizeof(header) + additional.size());

  auto *payload = buffer.prepare().data() + sizeof(goaway_frame);
  std::copy(std::begin(additional), std ::end(additional), payload);

  buffer.commit(sizeof(goaway_frame) + additional.size());
  return buffer;
}

utils::buffer ping(std::span<const uint8_t> additional) {
  utils::buffer buffer(sizeof(ping_frame));
  ping_frame *frame = reinterpret_cast<ping_frame *>(buffer.prepare().data());
  frame->type = frame_type::PING;
  frame->flags = 0;
  frame->stream_id = 0;
  frame->set_payload_size(sizeof(frame->data));
  memcpy(&frame->data, additional.data(), std::min(additional.size(), sizeof(frame->data)));

  buffer.commit(sizeof(ping_frame));
  return buffer;
}

utils::buffer reset(error_code err, boost::endian::big_uint32_t stream_id) {
  utils::buffer buffer(sizeof(reset_frame));
  reset_frame *frame = reinterpret_cast<reset_frame *>(buffer.prepare().data());
  frame->type = frame_type::RST_STREAM;
  frame->flags = 0;
  frame->set_payload_size(sizeof(reset_frame) - sizeof(header));
  frame->stream_id = stream_id;
  frame->code = boost::endian::native_to_big(err);

  buffer.commit(sizeof(reset_frame));
  return buffer;
}

utils::buffer update_window(uint32_t size, boost::endian::big_uint32_t stream_id) {
  utils::buffer buffer(sizeof(window_update_frame));
  window_update_frame *frame = reinterpret_cast<window_update_frame *>(buffer.prepare().data());
  frame->type = frame_type::WINDOW_UPDATE;
  frame->flags = 0;
  frame->set_payload_size(sizeof(window_update_frame) - sizeof(header));
  frame->stream_id = stream_id;
  frame->window_size = size;

  buffer.commit(sizeof(window_update_frame));
  return buffer;
}

utils::buffer settings(const std::vector<settings_item> &fields) {
  auto payload_size = sizeof(settings_item) * fields.size();
  utils::buffer buffer(sizeof(header) + payload_size);
  header *frame = reinterpret_cast<header *>(buffer.prepare().data());
  frame->type = frame_type::SETTINGS;
  frame->flags = 0;
  frame->stream_id = 0;
  frame->set_payload_size(payload_size);
  memcpy(buffer.prepare().data() + sizeof(header), fields.data(), payload_size);

  buffer.commit(sizeof(header) + payload_size);
  return buffer;
}

utils::buffer settings_ack() {
  utils::buffer buffer(sizeof(settings_frame));
  header *frame = reinterpret_cast<header *>(buffer.prepare().data());
  frame->type = frame_type::SETTINGS;
  frame->flags = flags::ACK;
  frame->stream_id = 0;
  frame->set_payload_size(0);
  buffer.commit(sizeof(settings_frame));

  return buffer;
}

utils::buffer headers(boost::endian::big_uint32_t stream_id, uint8_t flags, uint32_t payload_size) {
  auto size = sizeof(header) + (flags & flags::PADDED ? 1 : 0) + (flags & flags::PRIORITY ? 5 : 0);
  utils::buffer buffer(size);
  header *frame = reinterpret_cast<header *>(buffer.prepare().data());
  frame->type = frame_type::HEADERS;
  frame->flags = flags;
  frame->stream_id = stream_id;
  frame->set_payload_size(payload_size);

  // TODO: add padding, stream dependecie here

  buffer.commit(size);
  return buffer;
}

utils::buffer continuation(boost::endian::big_uint32_t stream_id, uint8_t flags, uint32_t payload_size) {
  auto size = sizeof(header);
  utils::buffer buffer(size);
  header *frame = reinterpret_cast<header *>(buffer.prepare().data());
  frame->type = frame_type::CONTINUATION;
  frame->flags = flags;
  frame->stream_id = stream_id;
  frame->set_payload_size(payload_size);

  buffer.commit(size);
  return buffer;
}

std::pair<utils::buffer, std::span<uint8_t>> data(boost::endian::big_uint32_t stream_id, uint8_t flags,
                                                  uint32_t payload_size) {
  auto size = sizeof(header) + (flags & flags::PADDED ? 1 : 0);
  utils::buffer buffer(size + payload_size);
  auto span = buffer.prepare();
  header *frame = reinterpret_cast<header *>(span.data());
  frame->type = frame_type::DATA;
  frame->flags = flags;
  frame->stream_id = stream_id;
  frame->set_payload_size(payload_size);

  buffer.commit(size + payload_size);

  return std::make_pair<utils::buffer, std::span<uint8_t>>(std::move(buffer), span.subspan(size, payload_size));
}

} // namespace http2::frame_builder
