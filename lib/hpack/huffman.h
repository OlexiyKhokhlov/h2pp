#pragma once

#include <cstdint>
#include <optional>
#include <span>

namespace rfc7541::huffman {

// FIXME: Do we really need here uint16_t?
using value_type = uint16_t;

constexpr value_type EOS = 256;

struct huffman_code {
  uint32_t huffmanCode;
  uint8_t bitLength;
};

std::optional<value_type> decode(const huffman_code &code);
const huffman_code &encode(value_type value);
const std::span<const uint8_t> allowed_code_lengths();

/**
 * @brief estimate_len
 * @param data
 * @return encoded data length in bits
 */
std::size_t estimate_len(std::span<const uint8_t> data);

} // namespace rfc7541::huffman
