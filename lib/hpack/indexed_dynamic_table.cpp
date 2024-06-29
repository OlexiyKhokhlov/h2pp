#include "indexed_dynamic_table.h"

#include <algorithm>
#include <compare>
#include <cstring>
#include <tuple>

namespace std {
std::strong_ordering operator<=>(const std::span<const uint8_t> lhs, const std::span<const uint8_t> rhs) {
  auto size_cmp = lhs.size_bytes() <=> rhs.size_bytes();
  if (size_cmp != std::strong_ordering::equal) {
    return size_cmp;
  }

  auto data_cmp = memcmp(lhs.data(), rhs.data(), rhs.size_bytes());
  if (data_cmp == 0) {
    return std::strong_ordering::equal;
  } else if (data_cmp < 0) {
    return std::strong_ordering::less;
  }
  return std::strong_ordering::greater;
}
} // namespace std

namespace rfc7541 {

bool indexed_dynamic_table::index_comparator::operator()(const record *lhs, const record *rhs) const {
  return std::tie(lhs->name, lhs->value) < std::tie(rhs->name, rhs->value);
}

bool indexed_dynamic_table::index_comparator::operator()(const record *lhs, const name_value_t &rhs) const {
  const auto name_span = lhs->name;
  const auto value_span = lhs->value;
  return std::tie(name_span, value_span) < std::tie(rhs.first, rhs.second);
}

void indexed_dynamic_table::insert(const std::span<const uint8_t> name, const std::span<const uint8_t> value) {
  record r{};
  r.name.assign(name.begin(), name.end());
  r.value.assign(value.begin(), value.end());
  auto hsize = r.hpack_size();

  auto rq_size = (hsize > hpack_max_size) ? 0 : hpack_max_size - hsize;
  auto it = shrink_to(rq_size);
  std::for_each(it, table.end(), [this](const auto &r) { index_set.erase(&r); });
  table.erase(it, table.end());

  insert_impl(std::move(r));
  index_set.insert(&table.front());
}

void indexed_dynamic_table::update_size(std::size_t size) {
  auto it = shrink_to(size);
  std::for_each(it, table.end(), [this](const auto &r) { index_set.erase(&r); });
  table.erase(it, table.end());
  hpack_max_size = size;
}

std::pair<int, bool> indexed_dynamic_table::field_index(const std::span<const uint8_t> name,
                                                        const std::span<const uint8_t> value) {
  auto it = index_set.lower_bound(std::make_pair(name, value));
  if (it == index_set.end()) {
    return {-1, false};
  }
  int i = -1;
  bool has_value = false;
  const auto *record = *it;
  using span_type = decltype(name);
  if (std::strong_ordering::equal == (span_type(record->name) <=> name)) {
    i = record_index(*it);
    has_value = std::strong_ordering::equal == (span_type(record->value) <=> value);
  }
  return {i, has_value};
}

} // namespace rfc7541
