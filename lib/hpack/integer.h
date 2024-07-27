#pragma once

#include <cstdint>
#include <span>

#include "constants.h"
#include "utils/utils.h"

namespace rfc7541::integer {

struct encoded_result {
  uint8_t value[5];
  uint8_t length = 0;

  std::span<const uint8_t> as_span() const noexcept { return {value, length}; }
};

encoded_result encode(uint8_t init, unsigned bitlen, uint32_t src_value);

inline encoded_result encode(command cmd, uint32_t src_value) {
  return encode(cmd_info::get(cmd).value, cmd_info::get(cmd).bitlen, src_value);
}

inline encoded_result encode(constants::string_flag f, uint32_t src_value) { return encode(uint8_t(f), 1, src_value); }

struct decoded_result {
  uint32_t used_bytes;
  uint32_t value;
};

decoded_result decode(unsigned bit_suffix_len, std::span<const uint8_t> src);

} // namespace rfc7541::integer
