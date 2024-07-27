#pragma once

#include <utils/streambuf.h>

#include "integer.h"

namespace rfc7541 {

class encoder_stream {
public:
  explicit encoder_stream(std::size_t max) : max_size(max), left(max) {}
  encoder_stream(const encoder_stream &) = delete;
  encoder_stream &operator=(const encoder_stream &) = delete;
  encoder_stream(encoder_stream &&) = delete;
  encoder_stream &operator=(encoder_stream &&) = delete;
  ~encoder_stream() = default;

  bool push_back(std::span<const uint8_t> src);

  void write_string(std::pair<std::size_t, constants::string_flag> estimation, integer::encoded_result encoded_size,
                    std::span<const uint8_t> src);

  auto flush() {
    left = max_size;
    return stream.flush();
  }
  std::size_t bytes_left() const noexcept { return left; }

private:
  void encode_string(const std::span<const uint8_t> src);

private:
  utils::streambuf stream;
  std::size_t max_size;
  std::size_t left;
};

} // namespace rfc7541
