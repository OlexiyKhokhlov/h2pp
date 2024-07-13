#pragma once

#include <compare>
#include <iterator>
#include <limits>
#include <type_traits>

namespace utils {

/**
 * @brief make_mask makes a bitwise mask with n '1'
 * @param n - bites count
 * @note when n is greater than bit-length the result is undefined
 * @return
 */
template <typename T, std::enable_if_t<std::numeric_limits<T>::is_integer && std::is_unsigned_v<T>, int> = 0>
constexpr T make_mask(unsigned n) {
  T init_mask = n > sizeof(T) * 8 - 1 ? 0 : 1;
  return (init_mask << n) - 1;
}

/**
 * @brief ceil_order2
 * @param value is a value that should be ceiled
 * @param order is order of 2
 * @return return a value that is not less that given value and is a multiple of
 * 2^order
 * @note if value is 0 always returns 0
 */
constexpr std::size_t ceil_order2(std::size_t value, unsigned order) {
  return (value + make_mask<size_t>(order)) & ~make_mask<size_t>(order);
}

} // namespace utils
