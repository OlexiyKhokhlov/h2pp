#include "header_field.h"

#include <stdexcept>

namespace {

template <typename T, typename V> void check_max_size(T t, V v) {
  // 2^24-1 - is a max frame size
  // len*8/5 - is a minimal size with huffman comperssion
  if ((t.size() + v.size()) * 8 / 5 >= (1 << 24) - 1) {
    throw std::overflow_error("Header is to big to be in the frame");
  }
}
} // namespace
namespace rfc7541 {

header_field::header_field(const std::span<const uint8_t> name, const std::span<const uint8_t> value, index_type type)
    : m_type(type) {
  check_max_size(name, value);
  m_name.assign(name.begin(), name.end());
  m_value.assign(value.begin(), value.end());
}

header_field::header_field(std::string_view name, std::string_view value, index_type type) : m_type(type) {
  check_max_size(name, value);
  m_name.assign(name.begin(), name.end());
  m_value.assign(value.begin(), value.end());
}

header_field::header_field(std::vector<uint8_t> &&name, std::vector<uint8_t> &&value, index_type type) : m_type(type) {
  check_max_size(name, value);
  m_name = std::move(name);
  m_value = std::move(value);
}

header_field::~header_field() {
  if (m_type == index_type::NEVER_INDEX) {
    std::fill(m_name.begin(), m_name.end(), 0);
    std::fill(m_value.begin(), m_value.end(), 0);
  }
}
} // namespace rfc7541
