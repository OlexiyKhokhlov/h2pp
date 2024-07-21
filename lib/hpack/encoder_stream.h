#pragma once

#include <utils/streambuf.h>

#include "integer.h"

namespace rfc7541 {

class encoder_stream : public utils::streambuf {
public:
  encoder_stream() = default;
  encoder_stream(const encoder_stream &) = delete;
  encoder_stream(encoder_stream &&) = delete;
  ~encoder_stream() = default;

  void encode_string(const std::span<const uint8_t> src);
  void write_string(std::pair<std::size_t, bool> estimation, integer::encoded_result encoded_size,
                    std::span<const uint8_t> src);
};

} // namespace rfc7541
