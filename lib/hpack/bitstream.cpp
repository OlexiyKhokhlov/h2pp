#include "bitstream.h"

#include <boost/endian/conversion.hpp>

#include <utils/utils.h>

namespace rfc7541 {

ibitstream::~ibitstream() = default;

uint32_t ibitstream::take_bits(std::size_t len) noexcept {
  // NOTE: len <= 32
  auto last_ind = (offset + len) / 8;
  auto last_gap = 8 - (offset + len) % 8;
  auto bytes_len = last_ind - offset / 8 + 1;
  auto first_gap = offset % 8;

  // NOTE: we can read 6 bytes before a real byte data by performance issue
  auto *first = reinterpret_cast<const uint64_t *>(data.data() + last_ind - 7);
  uint64_t bits = boost::endian::big_to_native(*first);

  bits &= utils::make_mask<uint64_t>(len) << last_gap;
  bits <<= (32 - bytes_len * 8 + first_gap);
  return bits;
}

void ibitstream::commit_bits(std::size_t bit_len) noexcept {
  offset = std::min(offset + bit_len, data.size_bytes() * 8);
}

} // namespace rfc7541
