#pragma once

#include "dynamic_table.h"
#include "hpack_exception.h"
#include "indexed_dynamic_table.h"
#include "static_table.h"

namespace rfc7541 {

template <typename DynTableT> class hpack_table {
public:
  explicit hpack_table(std::size_t max_size) : dyn_table(max_size) {}

  hpack_table() = delete;
  hpack_table(const dynamic_table &) = delete;
  hpack_table &operator=(const hpack_table &) = delete;
  hpack_table(hpack_table &&) = default;
  hpack_table &operator=(hpack_table &&) = default;
  ~hpack_table() = default;

  std::pair<std::span<const uint8_t>, std::span<const uint8_t>> at(std::size_t i) const {
    if (i == 0) {
      throw hpack_exception("Invalid index value 0", 0);
    }
    if (i <= static_table::size()) {
      return static_table::at(i);
    }
    return dyn_table.at(i - static_table::size());
  }

  void insert(const std::span<const uint8_t> name, const std::span<const uint8_t> value) {
    dyn_table.insert(name, value);
  }

  void update_size(std::size_t size) { dyn_table.update_size(size); }

  std::pair<int, bool> field_index(const std::span<const uint8_t> name, const std::span<const uint8_t> value) {
    auto [i, has_value] = static_table::field_index(name, value);
    if (has_value) {
      return std::make_pair(i, has_value);
    }
    auto [dyn_i, dyn_has_value] = dyn_table.field_index(name, value);
    if (dyn_has_value) {
      return std::make_pair(dyn_i + static_table::size(), dyn_has_value);
    } else if (i != -1) {
      return std::make_pair(i, has_value);
    }
    return std::make_pair(dyn_i, dyn_has_value);
  }

  std::size_t max_size() const { return dyn_table.max_size(); }

private:
  DynTableT dyn_table;
};

using decoder_table = hpack_table<dynamic_table>;
using encoder_table = hpack_table<indexed_dynamic_table>;

} // namespace rfc7541
