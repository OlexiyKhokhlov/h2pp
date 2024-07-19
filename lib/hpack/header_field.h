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
 * @brief The header_field class is a representation of HTTP single header.
 * A HTTP header is a pair of 'key' an 'value'.
 * RFC 7541 doen't have any limitation to the key and value elements.
 * So this class internaly handles `std::vector<uint8_t>` for both components.
 * All methods those manipulate with 'std::string' or 'std::string_view'
 * are just convient wrappers around corresponed 'std::vector' storage.
 *
 * @note a constructor can throw an std::overflow_error when a header_field is too big
 * to be inside a HTTP2 frame. but isn't a final check for that.
 *
 * A 'type' of header field is a HTTP\2 feature.
 * WITHOUT_INDEX - means a HPACK encoder should never use indexing for this header  filed.
 * It can be used if you know a heder field will never be repeated in this session or when this header field size is
 * closed to the dynamic table size. Actually it is a kind of HPACK optimisation. NEVER_INDEX - is the same as
 * WITHOUT_INDEX but in the case when a peer want to send it back it must not use indexing for this value. This type is
 * reuquired by security sensual data. So a value with this flag will never be stored in encoder/decoder tables.
 *
 * @note: RFC7540 (HTTP/1.1) describes generic HTTP filed name/value
 * limitations. RFC7540 (HTTP2) has also him own not so strict  limitaions for
 * that. But RFC7541 (HPACK) doesn't have anything relevant. So this
 * implementation doesn't have any restriction for name/value content.
 * Such restrictions should be implemented later on the layer up.
 */
class header_field {
public:
  header_field(const header_field &) = default;
  header_field &operator=(const header_field &) = default;
  header_field(header_field &&) = default;
  header_field &operator=(header_field &&) = default;
  ~header_field();

  header_field(const std::span<const uint8_t> name, const std::span<const uint8_t> value,
               index_type type = index_type::DEFAULT);
  header_field(std::string_view name, std::string_view value, index_type type = index_type::DEFAULT);
  header_field(std::vector<uint8_t> &&name, std::vector<uint8_t> &&value, index_type type = index_type::DEFAULT);

  [[nodiscard]] index_type type() const noexcept { return m_type; }

  [[nodiscard]] const std::span<const uint8_t> name() const noexcept { return m_name; }
  [[nodiscard]] const std::span<const uint8_t> value() const noexcept { return m_value; }

  [[nodiscard]] std::string_view name_view() const noexcept {
    return {reinterpret_cast<const char *>(m_name.data()), m_name.size()};
  }
  [[nodiscard]] std::string_view value_view() const {
    return {reinterpret_cast<const char *>(m_value.data()), m_value.size()};
  }

  /**
   * @brief hpack_size returns a HPACK specific size.
   * Should be used from encoder/decoder internals.
   */
  [[nodiscard]] auto hpack_size() const noexcept { return m_name.size() + m_value.size() + 32; }

protected:
  std::vector<uint8_t> m_name;
  std::vector<uint8_t> m_value;
  index_type m_type = index_type::DEFAULT;
};

using header = std::vector<header_field>;

} // namespace rfc7541
