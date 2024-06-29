#pragma once

#include <set>

#include "dynamic_table.h"

namespace rfc7541 {

class indexed_dynamic_table : public dynamic_table {
public:
  explicit indexed_dynamic_table(std::size_t max_size) : dynamic_table(max_size) {}

  indexed_dynamic_table() = delete;
  indexed_dynamic_table(const indexed_dynamic_table &) = delete;
  indexed_dynamic_table(indexed_dynamic_table &&) = default;
  ~indexed_dynamic_table() = default;

  void insert(const std::span<const uint8_t> name, const std::span<const uint8_t> value);
  void update_size(std::size_t size);

  std::pair<int, bool> field_index(const std::span<const uint8_t> name, const std::span<const uint8_t> value);

private:
  struct index_comparator {
    using is_transparent = void;

    bool operator()(const record *lhs, const record *rhs) const;
    using name_value_t = std::pair<const std::span<const uint8_t>, const std::span<const uint8_t>>;
    bool operator()(const record *lhs, const name_value_t &rhs) const;
  };

  std::set<const record *, index_comparator> index_set;
};
} // namespace rfc7541
