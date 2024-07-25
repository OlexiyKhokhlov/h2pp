#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace rfc7541 {

/**
 * @brief The ibitstream class is an auxilary class that retrieves hpack
 * elements from the data array.
 */
class ibitstream {
public:
  explicit ibitstream(const std::span<const uint8_t> sp) : data(sp){};
  ~ibitstream();

  std::size_t pos() const noexcept { return offset; }
  bool empty() const noexcept { return offset == data.size_bytes() * 8; }

  uint32_t take_bits(std::size_t bit_len) noexcept;
  void commit_bits(std::size_t bit_len) noexcept;

private:
  const std::span<const uint8_t> data;
  uint64_t offset = 0;
};

} // namespace rfc7541
