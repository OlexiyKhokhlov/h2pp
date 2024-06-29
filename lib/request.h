#pragma once

#include <chrono>
#include <deque>
#include <initializer_list>
#include <string>
#include <variant>
#include <vector>

#include "hpack/header_field.h"
#include "method.h"

namespace http2 {

class stream;
using namespace std::chrono_literals;

class request {
public:
  request() = default;
  request(const request &) = delete;
  request(request &&) = default;
  ~request() = default;

  request &method(http2::method);
  request &url(std::string_view);
  request &header(rfc7541::header_field &&);
  request &headers(std::initializer_list<rfc7541::header_field>);
  request &body(std::vector<uint8_t> &&);
  request &body(std::string &&);

  void check_valid() const;

  const std::deque<rfc7541::header_field> &get_headers() const { return header_list; };

  std::size_t body_size() const { return size; }
  std::deque<std::span<const uint8_t>> body_spans() const { return span_list; }

  auto timeout() const { return m_timeout; }

private:
  // calling from stream
  void commit_headers(std::size_t n);
  void commit_body(std::size_t n);

private:
  std::deque<rfc7541::header_field> header_list;

  using block_type = std::variant</*std::monostate, */ std::string, std::vector<uint8_t>>;
  std::deque<block_type> body_list;
  std::deque<std::span<const uint8_t>> span_list;
  std::size_t size = 0;
  const std::chrono::milliseconds m_timeout = 30s;

  friend stream;
};

} // namespace http2
