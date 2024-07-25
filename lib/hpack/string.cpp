#include "string.h"

#include "bitstream.h"
#include "constants.h"
#include "huffman.h"
#include "integer.h"

namespace rfc7541::string {

namespace {
std::vector<uint8_t> read_huffman_str(std::span<const uint8_t> src) {
  ibitstream stream(src);
  std::vector<uint8_t> result;

  // FIXME: len is an  encoded byte size.
  // The real size can be greater
  result.reserve(src.size_bytes() + src.size_bytes() / 2);

  auto codeLengths = rfc7541::huffman::allowed_code_lengths();
  while (!stream.empty()) {
    if (src.size_bytes() * 8 - stream.pos() < 8) {
      // Check EOS padding
      auto mask = utils::make_mask<uint8_t>(src.size_bytes() * 8 - stream.pos());
      if (mask == (src.back() & mask)) {
        return result;
      }
    }

    uint32_t hcode{};
    std::optional<rfc7541::huffman::value_type> index;
    uint8_t code_len{};
    for (auto bit_len : codeLengths) {
      hcode = stream.take_bits(bit_len);
      index = rfc7541::huffman::decode({hcode, bit_len});
      if (index) {
        code_len = bit_len;
        break;
      }
    }

    if (!index) {
      throw std::invalid_argument("Can't decode huffman code");
    }

    if (index.value() == rfc7541::huffman::EOS) {
      return result;
    }
    result.push_back(index.value());
    stream.commit_bits(code_len);
  }
  result.shrink_to_fit();
  return result;
}

} // namespace

decoded_result decode(std::span<const uint8_t> src) {
  if (src.empty()) {
    throw std::invalid_argument("A source can't be empty");
  }

  decoded_result result;

  bool is_huffman = src[0] & constants::ENCODED_STRING_FLAG;

  auto str_length = integer::decode(1, src);
  src = src.subspan(str_length.used_bytes);

  if (src.size_bytes() < str_length.value) {
    throw std::invalid_argument("Not enough input data for string");
  }

  if (!is_huffman) {
    result.value.assign(src.begin(), src.begin() + str_length.value);
  } else {
    result.value = read_huffman_str(src.subspan(0, str_length.value));
  }

  result.used_bytes = str_length.used_bytes + str_length.value;
  return result;
}

} // namespace rfc7541::string
