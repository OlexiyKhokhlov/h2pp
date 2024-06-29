#pragma once

#include <cstdint>

#include <boost/endian/conversion.hpp>

namespace utils {

// Thrue compile-time methods for conversion native-to-big

consteval uint32_t native_to_big(uint32_t x) {
  if constexpr (boost::endian::order::native == boost::endian::order::native) {
    return x;
  } else {
    std::uint32_t step16 = x << 16 | x >> 16;
    return ((step16 << 8) & 0xff00ff00) | ((step16 >> 8) & 0x00ff00ff);
  }
}

consteval uint16_t native_to_big(uint16_t x) {
  if constexpr (boost::endian::order::native == boost::endian::order::native) {
    return x;
  } else {
    return (x << 8) | (x >> 8);
  }
}

} // namespace utils
