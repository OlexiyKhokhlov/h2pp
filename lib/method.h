#pragma once

#include <string_view>

namespace http2 {

enum class method {
  ACL = 0,
  BASELINE_CONTROL,
  BIND,
  CHECKIN,
  CHECKOUT,
  CONNECT,
  COPY,
  Delete,
  GET,
  HEAD,
  LABEL,
  LINK,
  LOCK,
  MERGE,
  MKACTIVITY,
  MKCALENDAR,
  MKCOL,
  MKREDIRECTREF,
  MKWORKSPACE,
  MOVE,
  OPTIONS,
  ORDERPATCH,
  PATCH,
  POST,
  PRI,
  PROPFIND,
  PROPPATCH,
  PUT,
  REBIND,
  REPORT,
  SEARCH,
  TRACE,
  UNBIND,
  UNCHECKOUT,
  UNLINK,
  UNLOCK,
  UPDATE,
  UPDATEREDIRECTREF,
  VERSION_CONTROL,
};

std::string_view to_string(method cmd);

} // namespace http2
