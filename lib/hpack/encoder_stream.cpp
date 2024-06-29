#include "encoder_stream.h"

#include <boost/endian/conversion.hpp>

#include <utils/utils.h>

#include "constants.h"
#include "huffman.h"

namespace rfc7541 {

void encoder_stream::encode_string(const std::span<const uint8_t> src) {
  uint64_t val = 0;
  unsigned len = 0;
  for (auto byte : src) {
    auto code = huffman::encode(byte);
    auto bites_left = sizeof(val) * 8 - len;
    if (bites_left < code.bitLength) {
      auto big = boost::endian::native_to_big(val << bites_left);
      auto ready_bytes = len / 8;
      push_back({reinterpret_cast<const uint8_t *>(&big), ready_bytes});
      len = len - ready_bytes * 8;
    }
    val <<= code.bitLength;
    // TODO: rebuild codes without offset to 32bit
    val |= (static_cast<uint64_t>(code.huffmanCode) >> (32 - code.bitLength));
    len += code.bitLength;
  }

  if (len > 0) {
    // Store tail of encoded data
    unsigned bites_left = sizeof(val) * 8 - len;
    val <<= bites_left;
    val |= utils::make_mask<uint64_t>(bites_left);
    auto big = boost::endian::native_to_big(val);
    push_back({reinterpret_cast<const uint8_t *>(&big), utils::ceil_order2(len, 3) / 8});
  }
}

void encoder_stream::write_string(std::pair<std::size_t, bool> estimation, encoded_integer encoded_size,
                                  std::span<const uint8_t> src) {

  if (estimation.second) {
    // Encoded string
    encoded_size.value |= constants::ENCODED_STRING_FLAG;
    push_back(encoded_size.as_span());
    encode_string(src);
  } else {
    // Save as is
    push_back(encoded_size.as_span());
    push_back(src);
  }
}

} // namespace rfc7541
