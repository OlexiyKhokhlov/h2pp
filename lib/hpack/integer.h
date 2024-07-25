#pragma once

#include <cstdint>
#include <span>

#include "utils/utils.h"

namespace rfc7541::integer {

struct encoded_result {
  uint32_t length;
  uint32_t value;

  std::span<const uint8_t> as_span() const noexcept { return {reinterpret_cast<const uint8_t *>(&value), length}; }
};

encoded_result encode(unsigned bit_suffix_len, uint32_t src_value);

struct decoded_result {
  uint32_t used_bytes;
  uint32_t value;
};

decoded_result decode(unsigned bit_suffix_len, std::span<const uint8_t> src);

} // namespace rfc7541::integer
