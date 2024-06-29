#pragma once

#include <cstdint>
#include <deque>
#include <span>

#include <utils/buffer.h>

#include "header_field.h"
#include "hpack_table.h"

namespace rfc7541 {

class encoder {
public:
  struct encoder_config {
    ~encoder_config() = default;
    encoder_config() = default;
    encoder_config(const encoder_config &) = default;
    encoder_config(encoder_config &&) = default;
    encoder_config &operator=(const encoder_config &) = default;

    std::size_t init_table_size = 4096;
    std::size_t max_table_size = 4096 * 4;
    std::size_t max_header_list_size = 0; // unlimited by default

    std::size_t min_huffman_rate = 90;        // in percents
    std::size_t min_dyntable_value_rate = 90; // in percents
  };

  encoder();
  encoder(const encoder &) = delete;
  encoder(encoder &&) = default;
  ~encoder() = default;

  void set_config(const encoder_config &conf) { config = conf; }
  const encoder_config &get_config() const { return config; }

  std::pair<std::deque<utils::buffer>, std::size_t> encode(std::deque<rfc7541::header_field> fields,
                                                           std::size_t size_limit);

private:
  std::pair<std::size_t, bool> estimate_string_size(std::span<const uint8_t> src);

private:
  encoder_config config;
  encoder_table table;
};

} // namespace rfc7541
