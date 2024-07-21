#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "constants.h"

namespace rfc7541 {

/**
 * @brief The ibitstream class is an auxilary class that retrieves hpack
 * elements from the data array.
 */
class ibitstream {
public:
  explicit ibitstream(const std::span<const uint8_t> sp) : data(sp){};
  ~ibitstream() = default;

  std::size_t bit_pos() const { return offset; }
  bool is_finished() const { return data.size_bytes() == offset / 8; }

  ibitstream &operator>>(command &cmd);
  ibitstream &operator>>(uint32_t &integer);
  ibitstream &operator>>(std::vector<uint8_t> &vector);

private:
  const std::span<const uint8_t> data;
  uint32_t offset = 0;

  uint32_t take_bits(std::size_t bit_len);
  void commit_bits(std::size_t bit_len);
  std::vector<uint8_t> read_huffman_str(std::size_t len);

  void check_enought_bytes(std::size_t bytes) const;
  void check_enought_bits(std::size_t bites) const;
};

} // namespace rfc7541
