#pragma once

#include <cstdint>

#include <boost/system/error_category.hpp>
#include <boost/system/is_error_code_enum.hpp>
#include <boost/system/system_error.hpp>

#include "utils/endianess.h"

namespace http2 {
enum class error_code : uint32_t {
  IS_OK = 0x0,
  PROTOCOL_ERROR = utils::native_to_big(1u),
  INTERNAL_ERROR = utils::native_to_big(2u),
  FLOW_CONTROL_ERROR = utils::native_to_big(3u),
  SETTINGS_TIMEOUT = utils::native_to_big(4u),
  STREAM_CLOSED = utils::native_to_big(5u),
  FRAME_SIZE_ERROR = utils::native_to_big(6u),
  REFUSED_STREAM = utils::native_to_big(7u),
  CANCEL = utils::native_to_big(8u),
  COMPRESSION_ERROR = utils::native_to_big(9u),
  CONNECT_ERROR = utils::native_to_big(10u),
  ENHANCE_YOUR_CALM = utils::native_to_big(11u),
  INADEQUATE_SECURITY = utils::native_to_big(12u),
  HTTP_1_1_REQUIRED = utils::native_to_big(13u),
};
}

namespace boost::system {

template <> struct is_error_code_enum<::http2::error_code> : std::true_type {};

} // namespace boost::system

namespace std {

template <> struct is_error_code_enum<::http2::error_code> : std::true_type {};

} // namespace std

namespace http2 {

class error_category_impl : public boost::system::error_category {
public:
  const char *name() const noexcept;

  std::string message(int ev) const;
  char const *message(int ev, char *buffer, std::size_t len) const noexcept;

  bool failed(int ev) const noexcept;
};

boost::system::error_category const &http2_category();

inline boost::system::error_code make_error_code(error_code e) {
  return boost::system::error_code(static_cast<int>(e), http2_category());
}

} // namespace http2
