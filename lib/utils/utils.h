#pragma once

#include <compare>
#include <iterator>
#include <limits>
#include <type_traits>

namespace utils {

template <typename T, std::enable_if_t<std::numeric_limits<T>::is_integer, int> = 0> constexpr T make_mask(unsigned n) {
  return (((T)1) << n) - 1;
}

/**
 * @brief ceil_order2
 * @param value
 * @param order is order of 2
 * @return return a value that is not less that given value and is a multiple of
 * 2^order
 */
constexpr std::size_t ceil_order2(std::size_t value, unsigned order) {
  return (value + make_mask<size_t>(order)) & ~make_mask<size_t>(order);
}

// TODO: https://habr.com/ru/companies/ruvds/articles/756422/
template<typename ForwardIt, typename T, typename Compare>
ForwardIt binary_search(ForwardIt first, ForwardIt last, const T &value, Compare comp)
{
    if (std::is_lt(comp(value, *first)) || std::is_gt(comp(value, *(last - 1)))) {
        return last;
    }

    auto end = last;
    do {
        auto diapason = std::distance(first, last);
        if (diapason == 1) {
            return std::is_eq(comp(value, *first)) ? first : end;
        }
        auto it = first + diapason / 2;
        const std::strong_ordering ordering = comp(value, *it);
        if (std::is_lt(ordering)) {
            last = it;
        } else if (std::is_gt(ordering)) {
            first = it;
        } else {
            return it;
        }
    } while (true);
    return end;
};

} // namespace utils
