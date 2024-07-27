#include "integer.h"

#include <stdexcept>

constexpr auto MAX_HPACK_INT = (1 << 24) - 16;

namespace rfc7541::integer {

encoded_result encode(uint8_t init, unsigned bitlen, uint32_t src_value) {
  if (src_value > MAX_HPACK_INT) {
    throw std::overflow_error("A value must be less than 2^24-1");
  }

  encoded_result result;
  result.value[0] = init;

  result.length = 1;
  uint8_t mask = utils::make_mask<uint8_t>(8 - bitlen);
  if (src_value < mask) {
    result.value[0] |= uint8_t(src_value) & mask;
    return result;
  }

  result.value[0] |= mask;
  src_value -= mask;

  while (src_value >= 0x80) {
    result.value[result.length] = uint8_t(src_value) & 0x7f;
    result.value[result.length] |= 0x80;
    src_value /= 128;
    result.length++;
  };

  result.value[result.length++] = uint8_t(src_value) & 0x7f;
  return result;
}

decoded_result decode(unsigned bit_suffix_len, std::span<const uint8_t> src) {
  if (src.empty() || bit_suffix_len > 4 || bit_suffix_len == 0) {
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
      throw std::overflow_error("An overflow in HPACK int creation");
    }
    m += 7;
  } while (b & 0x80);

  return result;
}

} // namespace rfc7541::integer
