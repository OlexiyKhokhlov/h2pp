#pragma once

#include <cstdint>
#include <limits>
#include <string_view>

#include "utils/endianess.h"

namespace http2 {

constexpr std::size_t INITIAL_WINDOW_SIZE = 65535;

enum class frame_type : uint8_t {
  DATA = 0x0,
  HEADERS = 0x1,
  PRIORITY = 0x2,
  RST_STREAM = 0x3,
  SETTINGS = 0x4,
  PUSH_PROMISE = 0x5,
  PING = 0x6,
  GOAWAY = 0x7,
  WINDOW_UPDATE = 0x8,
  CONTINUATION = 0x9,
  // LAST = 0xa,
};

enum class settings_type : uint16_t {
  HEADER_TABLE_SIZE = utils::native_to_big(1u),
  ENABLE_PUSH = utils::native_to_big(2u),
  MAX_CONCURRENT_STREAMS = utils::native_to_big(3u),
  INITIAL_WINDOW_SIZE = utils::native_to_big(4u),
  MAX_FRAME_SIZE = utils::native_to_big(5u),
  MAX_HEADER_LIST_SIZE = utils::native_to_big(6u),
  // `RFC 8441 <https://tools.ietf.org/html/rfc8441>`
  ENABLE_CONNECT_PROTOCOL = utils::native_to_big(8u),
  /**
   * SETTINGS_NO_RFC7540_PRIORITIES (:rfc:`9218`)
   */
  // NGHTTP2_SETTINGS_NO_RFC7540_PRIORITIES = 0x09
};

enum class push_state : uint32_t {
  DISABLED = 0,
  ENABLED = utils::native_to_big(1u),
};

enum class connection_protocol_state : uint32_t {
  DISABLED = 0,
  ENABLED = utils::native_to_big(1u),
};

struct settings {
  uint32_t header_table_size = 4096; //
  push_state enable_push = push_state::DISABLED;
  uint32_t max_concurrent_streams = 100;
  uint32_t initial_window_size = INITIAL_WINDOW_SIZE;
  uint32_t max_frame_size = 16384; // Should be between 2^14..2^24-1
  uint32_t max_header_list_size = std::numeric_limits<uint32_t>::max();
  connection_protocol_state connection_protocol = connection_protocol_state::DISABLED;
};

namespace flags {
constexpr uint8_t END_STREAM = 0x1;
constexpr uint8_t ACK = 0x1;
constexpr uint8_t END_HEADERS = 0x4;
constexpr uint8_t PADDED = 0x8;
constexpr uint8_t PRIORITY = 0x20;

constexpr uint8_t DATA_ALLOWED_FLAGS_MASK = END_STREAM | PADDED;
constexpr uint8_t HEADERS_ALLOWED_FLAGS_MASK = END_STREAM | END_HEADERS | PADDED | PRIORITY;
constexpr uint8_t PING_ALLOWED_FLAGS_MASK = ACK;
constexpr uint8_t PUSH_PROMISE_ALLOWED_FLAGS_MASK = END_HEADERS | PADDED;
constexpr uint8_t CONTINUATION_ALLOWED_FLAGS_MASK = END_HEADERS;

}; // namespace flags

} // namespace http2
