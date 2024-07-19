#include "error.h"

namespace http2 {

const char *error_category_impl::name() const noexcept { return "HTTP2"; }

std::string error_category_impl::message(int ev) const {
  std::string msg(64, '\0');
  message(ev, msg.data(), 64);
  return msg;
}

char const *error_category_impl::message(int ev, char *buffer, std::size_t len) const noexcept {
  const char *str_error = nullptr;
  switch (static_cast<error_code>(ev)) {
  case error_code::IS_OK:
    str_error = "No error";
    break;
  case error_code::PROTOCOL_ERROR:
    str_error = "Protocol error";
    break;
  case error_code::INTERNAL_ERROR:
    str_error = "Internal error";
    break;
  case error_code::FLOW_CONTROL_ERROR:
    str_error = "Flow control error";
    break;
  case error_code::SETTINGS_TIMEOUT:
    str_error = "Settings timeout";
    break;
  case error_code::STREAM_CLOSED:
    str_error = "Stream closed";
    break;
  case error_code::FRAME_SIZE_ERROR:
    str_error = "Frame size error";
    break;
  case error_code::REFUSED_STREAM:
    str_error = "Refused stream";
    break;
  case error_code::CANCEL:
    str_error = "Canceled";
    break;
  case error_code::COMPRESSION_ERROR:
    str_error = "Compression error";
    break;
  case error_code::CONNECT_ERROR:
    str_error = "Connect error";
    break;
  case error_code::ENHANCE_YOUR_CALM:
    str_error = "Enhace your calm";
    break;
  case error_code::INADEQUATE_SECURITY:
    str_error = "Inadequate security";
    break;
  case error_code::HTTP_1_1_REQUIRED:
    str_error = "HTTP1.1 required";
    break;
  default:
    std::snprintf(buffer, len, "Unknown HTTP2 error %d", ev);
    return buffer;
  }

  std::copy_n(str_error, std::min(len, strlen(str_error)), buffer);
  return buffer;
}

bool error_category_impl::failed(int ev) const noexcept { return static_cast<error_code>(ev) != error_code::IS_OK; }

boost::system::error_category const &http2_category() {
  static const http2::error_category_impl instance;
  return instance;
}

} // namespace http2
