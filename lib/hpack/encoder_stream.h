#pragma once

#include <utils/streambuf.h>

namespace rfc7541 {

struct encoded_integer {
  std::size_t length;
  uint64_t value;

  std::span<const uint8_t> as_span() const { return {reinterpret_cast<const uint8_t *>(&value), length}; }
};

class encoder_stream : public utils::streambuf {
public:
  encoder_stream() = default;
  encoder_stream(const encoder_stream &) = delete;
  encoder_stream(encoder_stream &&) = delete;
  ~encoder_stream() = default;

  void encode_string(const std::span<const uint8_t> src);
  void write_string(std::pair<std::size_t, bool> estimation, encoded_integer encoded_size,
                    std::span<const uint8_t> src);
};

} // namespace rfc7541
