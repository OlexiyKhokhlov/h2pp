#pragma once

#include <utility>
#include <vector>

#include "frame.h"
#include "utils/buffer.h"

namespace http2::frame_builder {

utils::buffer goaway(error_code err, boost::endian::big_uint32_t last_stream_id = 3372220416,
                     std::span<const uint8_t> additional = std::span<const uint8_t>{});

utils::buffer ping(std::span<const uint8_t> additional = std::span<const uint8_t>{});

utils::buffer reset(error_code err, boost::endian::big_uint32_t stream_id = 0);
utils::buffer update_window(uint32_t size, boost::endian::big_uint32_t steram_id = 0);
utils::buffer settings(const std::vector<settings_item> &fields);
utils::buffer settings_ack();

utils::buffer headers(boost::endian::big_uint32_t stream_id, uint8_t flags, uint32_t payload_size);
utils::buffer continuation(boost::endian::big_uint32_t stream_id, uint8_t flags, uint32_t payload_size);

std::pair<utils::buffer, std::span<uint8_t>> data(boost::endian::big_uint32_t stream_id, uint8_t flags,
                                                  uint32_t payload_size);
} // namespace http2::frame_builder
