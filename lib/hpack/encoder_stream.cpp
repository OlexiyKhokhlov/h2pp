#include "encoder_stream.h"

#include <boost/endian/conversion.hpp>

#include <utils/utils.h>

#include "constants.h"
#include "huffman.h"

namespace rfc7541 {

bool encoder_stream::push_back(std::span<const uint8_t> src) {
  if (src.size() <= left) {
    left -= src.size();
    stream.push_back(src);
    return true;
  }
  return false;
}

void encoder_stream::encode_string(const std::span<const uint8_t> src) {
  uint64_t val = 0;
  unsigned len = 0;
  for (auto byte : src) {
    auto code = huffman::encode(byte);
    auto bites_left = sizeof(val) * 8 - len;
    if (bites_left < code.bitLength) {
      auto big = boost::endian::native_to_big(val << bites_left);
      auto ready_bytes = len / 8;
      stream.push_back({reinterpret_cast<const uint8_t *>(&big), ready_bytes});
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
    stream.push_back({reinterpret_cast<const uint8_t *>(&big), utils::ceil_order2(len, 3) / 8});
  }
}

void encoder_stream::write_string(std::pair<std::size_t, constants::string_flag> estimation,
                                  integer::encoded_result encoded_size, std::span<const uint8_t> src) {

  if (estimation.second == constants::string_flag::ENCODED) {
    // Encoded string
    stream.push_back(encoded_size.as_span());
    encode_string(src);
  } else {
    // Save as is
    stream.push_back(encoded_size.as_span());
    stream.push_back(src);
  }
}

} // namespace rfc7541
