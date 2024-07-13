#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace rfc7541 {

enum class index_type : unsigned {
  DEFAULT,
  WITHOUT_INDEX,
  NEVER_INDEX,
};

/**
 *  NOTE: RFC7540 (HTTP/1.1) describes generic HTTP filed name/value
 * limitations. RFC7540 (HTTP2) has also him own not so strict  limitaions for
 * that. But RFC7541 (HPACK) doesn't have anything relevant. So this
 * implementation doesn't have any restriction for name/value content.
 * Such restrictions should be implemented later on the layer up.
 */

class header_field {
public:
  explicit header_field(index_type type = index_type::DEFAULT) : m_type(type) {}

  header_field(const header_field &) = default;
  header_field &operator=(const header_field &) = default;
  header_field(header_field &&) = default;
  header_field &operator=(header_field &&) = default;
  ~header_field();

  header_field(const std::span<const uint8_t> name, const std::span<const uint8_t> value,
                         index_type type = index_type::DEFAULT)
      : m_name(name.begin(), name.end()), m_value(value.begin(), value.end()), m_type(type) {}

  header_field(std::string_view name, std::string_view value, index_type type = index_type::DEFAULT)
      : m_name(name.begin(), name.end()), m_value(value.begin(), value.end()), m_type(type) {}

  header_field(std::vector<uint8_t> &&name, std::vector<uint8_t> &&value, index_type type = index_type::DEFAULT)
      : m_name(std::move(name)), m_value(std::move(value)), m_type(type) {}

  index_type type() const { return m_type; }

  const std::span<const uint8_t> name() const { return m_name; }
  const std::span<const uint8_t> value() const { return m_value; }

  void set_name(std::vector<uint8_t> &&name) { m_name = std::move(name); };

  void set_value(std::vector<uint8_t> &&value) { m_value = std::move(value); };

  std::string_view name_view() const { return {reinterpret_cast<const char *>(m_name.data()), m_name.size()}; }

  std::string_view value_view() const { return {reinterpret_cast<const char *>(m_value.data()), m_value.size()}; }

  auto hpack_size() const { return m_name.size() + m_value.size() + 32; }

protected:
  std::vector<uint8_t> m_name;
  std::vector<uint8_t> m_value;
  index_type m_type = index_type::DEFAULT;
};

using header = std::vector<header_field>;

} // namespace rfc7541
