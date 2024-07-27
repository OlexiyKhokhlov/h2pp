#pragma once

#include <cstdint>

namespace rfc7541 {

namespace constants {

enum class string_flag : uint8_t {
  INPLACE = 0,
  ENCODED = 0x80,
};
}

enum class command : unsigned {
  INDEX = 0,                 // Contains an index for a pair name + value
  LITERAL_INCREMENTAL_INDEX, // Name index + encoded val | encoded name +
  // encoded val. Updates dynamic tale.
  CHANGE_TABLE_SIZE,
  LITERAL_WITHOUT_INDEX, // Name index + encoded val | encoded name + encoded
  // val. Do not updates dynamic table.
  LITERAL_NEVER_INDEX, // Name index + encoded val | encoded name + encoded val.
  // Do not updates dynamic table. + IS A MARK DO NOT USE
  // INDEX FUTHREMORE
};

namespace cmd_info {
struct info {
  uint8_t value;
  uint8_t bitlen;
  uint8_t mask;
};

constexpr info infos[] = {
    {0x80, 1, 0x80}, // command::INDEX
    {0x40, 2, 0xc0}, // command::LITERAL_INCREMENTAL_INDEX
    {0x20, 3, 0xe0}, // command::CHANGE_TABLE_SIZE
    {0x00, 4, 0xf0}, // command::LITERAL_WITHOUT_INDEX
    {0x10, 4, 0xf0}, // command::LITERAL_NEVER_INDEX
};

constexpr info get(command id) { return infos[static_cast<unsigned>(id)]; }
} // namespace cmd_info

} // namespace rfc7541
