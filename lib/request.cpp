#include "request.h"

namespace http2 {

namespace {
template <typename T> request &generic_url(request &owner, T url) {
  if (url.scheme().empty()) {
    throw std::invalid_argument("Empty scheme");
  }

  if (url.authority().empty()) {
    throw std::invalid_argument("Empty authority");
  }

  if (url.path().empty()) {
    throw std::invalid_argument("Empty path");
  }

  return owner.header({":scheme", url.scheme()})
      .header({":authority", url.encoded_authority()})
      .header({":path", url.encoded_resource()});
}
} // namespace

request::request(boost::url_view url_view, http2::method m) {
  generic_url<boost::url_view>(*this, url_view);
  header({":method", to_string(m)});
}

request::request(const boost::url &u, http2::method m) {
  generic_url<boost::url_view>(*this, u);
  header({":method", to_string(m)});
}

request::~request() = default;

request::request(request &&rhs)
    : header_list(std::move(rhs.header_list)), body_list(std::move(rhs.body_list)), span_list(std::move(rhs.span_list)),
      size(rhs.size), timeout_value(rhs.timeout_value) {
  rhs.size = 0;
}

request &request::operator=(request &&rhs) {
  header_list = std::move(rhs.header_list);
  body_list = std::move(rhs.body_list);
  span_list = std::move(rhs.span_list);
  size = rhs.size;
  timeout_value = rhs.timeout_value;

  rhs.size = 0;
  return *this;
}

request &request::header(rfc7541::header_field &&h) {
  header_list.emplace_back(std::move(h));
  return *this;
}
request &request::headers(std::initializer_list<rfc7541::header_field> fields) {
  std::copy(fields.begin(), fields.end(), std::back_inserter(header_list));
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
