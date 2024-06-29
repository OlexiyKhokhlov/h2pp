#pragma once

#include <deque>
#include <ranges>
#include <span>

#include "hpack/header_field.h"
#include "utils/buffer.h"

namespace http2 {

class stream;

class response {
public:
  response() = default;
  response(const response &) = delete;
  response &operator=(const response &) = delete;
  response(response &&) = default;
  response &operator=(response &&) = default;
  ~response() = default;

  std::string_view status() const { return status_view; }
  const std::deque<rfc7541::header_field> &headers() const { return header_list; };

  std::size_t body_size() const { return size; }

  auto body_range() const {
    return body_blocks | std::views::transform([](const auto &b) { return b.span; }) | std::views::join;
  }

  template <typename Container> [[nodiscard]] auto body_as() const {
    Container result;
    result.resize(body_size() / sizeof(typename Container::value_type));
    copy_body(static_cast<char *>(result.data()), body_size());
    return result;
  }

private:
  void insert_headers(rfc7541::header &&header);
  void insert_body(utils::buffer &&);
  std::size_t copy_body(char *dst, std::size_t len) const;

private:
  std::deque<rfc7541::header_field> header_list;
  mutable std::string_view status_view;

  struct body_block {
    utils::buffer buffer;
    std::span<const uint8_t> span;
  };

  std::deque<body_block> body_blocks;
  std::size_t size = 0;

  friend stream;
};

} // namespace http2
