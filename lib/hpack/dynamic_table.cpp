#include "dynamic_table.h"

#include <cstring>
#include <stdexcept>

namespace rfc7541 {

std::pair<std::span<const uint8_t>, std::span<const uint8_t>> dynamic_table::at(std::size_t i) const {
  if (i <= table.size()) {
    const auto &record = table[i - 1];
    return {record.name, record.value};
  }

  // FIXME: offset
  throw std::runtime_error("Invalid index");
}

void dynamic_table::insert(const std::span<const uint8_t> name, const std::span<const uint8_t> value) {
  record r{};
  r.name.assign(name.begin(), name.end());
  r.value.assign(value.begin(), value.end());
  auto hsize = r.hpack_size();

  auto rq_size = ((hsize + table_size) > hpack_max_size) ? hpack_max_size - hsize : hsize + table_size;
  table.erase(shrink_to(rq_size), table.end());

  insert_impl(std::move(r));
}

void dynamic_table::insert_impl(record &&r) {
  unsigned index = 0;
  if (!table.empty()) {
    index = table.front().init_index + 1;
  }

  r.init_index = index;
  auto hsize = r.hpack_size();
  table.push_front(std::move(r));
  table_size += hsize;
}

void dynamic_table::update_size(std::size_t size) {
  table.erase(shrink_to(size), table.end());
  hpack_max_size = size;
}

dynamic_table::table_iterator dynamic_table::shrink_to(std::size_t rq_size) {
  if (table_size <= rq_size) {
    return table.end();
  }

  auto eviction_size = table_size - rq_size;
  auto it = table.end();
  do {
    it--;
    auto sz = it->hpack_size();
    eviction_size -= sz;
    table_size -= sz;
  } while (it != table.begin() && eviction_size > 0);

  return it;
}

std::size_t dynamic_table::record_index(const record *r) const {
  return table.size() + table.back().init_index - r->init_index;
}

} // namespace rfc7541
