#include "request.h"

#include <boost/url.hpp>

namespace http2 {
request &request::method(http2::method m) {
  return headers({
      {":method", to_string(m)},
  });
}

request &request::url(std::string_view url_string) {
  auto url = boost::urls::parse_uri(url_string);
  if (url.has_error()) {
    throw std::invalid_argument("invalid url");
  }

  return headers({
      {":scheme", url->scheme()},
      {":authority", url->authority().buffer()},
      {":path", url->path() + (url->query().empty() ? "" : '?' + url->query()) +
                    (url->fragment().empty() ? "" : '#' + url->fragment())},
  });
}

request &request::header(rfc7541::header_field &&h) {
  header_list.emplace_back(std::move(h));
  return *this;
}
request &request::headers(std::initializer_list<rfc7541::header_field> fields) {
  std::move(fields.begin(), fields.end(), std::back_inserter(header_list));
  return *this;
}

request &request::body(std::vector<uint8_t> &&buffer) {
  body_list.emplace_back(std::move(buffer));
  const auto &b = std::get<std::vector<uint8_t>>(body_list.back());
  std::span<const uint8_t> span = b;
  span_list.emplace_back(span);
  size += span.size_bytes();
  return *this;
}
request &request::body(std::string &&str) {
  body_list.emplace_back(std::move(str));
  const auto &s = std::get<std::string>(body_list.back());
  std::span<const uint8_t> span(reinterpret_cast<const uint8_t *>(s.data()), s.size());
  span_list.emplace_back(span);
  size += span.size_bytes();
  return *this;
}

void request::check_valid() const {
  if (header_list.size() < 4) {
    throw std::invalid_argument("Request doesn't have all required headers");
  }
}

void request::commit_headers(std::size_t n) {
  while (n--) {
    header_list.pop_front();
  };
}

void request::commit_body(std::size_t n) {
  while (n--) {
    body_list.pop_front();
    span_list.pop_front();
  };
}

} // namespace http2
