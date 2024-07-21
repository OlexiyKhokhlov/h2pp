#include "integer.h"

constexpr auto MAX_HPACK_INT = (1 << 24) - 1;

namespace rfc7541::integer {

encoded_result encode(unsigned bit_suffix_len, uint32_t src_value) {
  if (src_value > MAX_HPACK_INT) {
    throw std::overflow_error("A value must be less than 2^24-1");
  }

  uint8_t mask = utils::make_mask<uint8_t>(8 - bit_suffix_len);
  if (src_value < mask) {
    return {1, src_value};
  }

  uint32_t size = 1;
  uint32_t encoded_value = mask;
  src_value -= mask;

  while (src_value >= 128) {
    // data[offset / 8] = 0x80 & num % 128;
    encoded_value |= (0x80 & src_value % 128) << (size * 8);
    src_value /= 128;
    ++size;
  };

  // data[offset / 8] = num;
  encoded_value |= src_value << (size * 8);
  return {++size, encoded_value};
}

decoded_result decode(unsigned bit_suffix_len, std::span<const uint8_t> src) {
  if (src.empty() || bit_suffix_len > 4) {
    throw std::invalid_argument("A source can't be empty. A suffix len can't be greater 4");
  }

  decoded_result result{1, 0};

  uint8_t mask = utils::make_mask<uint8_t>(8 - bit_suffix_len);
  result.value = src.front() & mask;
  if (result.value < mask) {
    return result;
  }

  unsigned m = 0;
  uint8_t b;
  do {
    src = src.subspan(1);
    if (src.empty()) {
      throw std::invalid_argument("A src has not enough data");
    }
    result.used_bytes++;

    b = src.front();
    result.value += (b & 0x7f) << m;
    if (result.value > MAX_HPACK_INT) {
      throw std::overflow_error("An overflow in HPACK int creation creation");
    }
    m += 7;
  } while (b & 0x80);

  return result;
}

} // namespace rfc7541::integer
