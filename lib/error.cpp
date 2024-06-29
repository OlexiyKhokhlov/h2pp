#include "error.h"

namespace http2 {

const char *error_category_impl::name() const noexcept { return "HTTP2"; }

std::string error_category_impl::message(int ev) const {
  std::string msg(64, '\0');
  message(ev, msg.data(), 64);
  return msg;
}

char const *error_category_impl::message(int ev, char *buffer, std::size_t len) const noexcept {
  switch (static_cast<error_code>(ev)) {
  case error_code::IS_OK:
    return "No error";
  case error_code::PROTOCOL_ERROR:
    return "Protocol error";
  case error_code::INTERNAL_ERROR:
    return "internal error";
  case error_code::FLOW_CONTROL_ERROR:
    return "Flow control error";
  case error_code::SETTINGS_TIMEOUT:
    return "Settings timeout";
  case error_code::STREAM_CLOSED:
    return "Stream closed";
  case error_code::FRAME_SIZE_ERROR:
    return "Frame size error";
  case error_code::REFUSED_STREAM:
    return "Refused stream";
  case error_code::CANCEL:
    return "Canceled";
  case error_code::COMPRESSION_ERROR:
    return "Compression error";
  case error_code::CONNECT_ERROR:
    return "Connect error";
  case error_code::ENHANCE_YOUR_CALM:
    return "Enhace your calm";
  case error_code::INADEQUATE_SECURITY:
    return "Inadequate security";
  case error_code::HTTP_1_1_REQUIRED:
    return "HTTP1.1 required";
  }

  std::snprintf(buffer, len, "Unknown HTTP2 error %d", ev);
  return buffer;
}

bool error_category_impl::failed(int ev) const noexcept { return static_cast<error_code>(ev) != error_code::IS_OK; }

boost::system::error_category const &http2_category() {
  static const http2::error_category_impl instance;
  return instance;
}

} // namespace http2
