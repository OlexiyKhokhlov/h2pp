#pragma once

#include <cstdint>
#include <deque>
#include <span>
#include <vector>

namespace rfc7541 {

class dynamic_table {
public:
  explicit dynamic_table(std::size_t max_size) : hpack_max_size(max_size) {}

  dynamic_table() = delete;
  dynamic_table(const dynamic_table &) = delete;
  dynamic_table(dynamic_table &&) = default;
  ~dynamic_table() = default;

  std::pair<std::span<const uint8_t>, std::span<const uint8_t>> at(std::size_t i) const;
  void insert(const std::span<const uint8_t> name, const std::span<const uint8_t> value);
  void update_size(std::size_t size);
  std::size_t max_size() const { return hpack_max_size; }

protected:
  struct record {
    std::vector<uint8_t> name;
    std::vector<uint8_t> value;
    unsigned init_index = 0;

    std::size_t hpack_size() const { return name.size() + value.size() + 32; }
  };
  std::size_t table_size = 0;
  std::size_t hpack_max_size = 4096;
  std::deque<record> table;

  using table_iterator = decltype(table.begin());
  table_iterator shrink_to(std::size_t size);
  void insert_impl(record &&r);
  std::size_t record_index(const record *r) const;
};
} // namespace rfc7541
