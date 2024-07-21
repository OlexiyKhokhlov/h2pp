#include "bitstream.h"

#include <vector>

#include <boost/endian/conversion.hpp>

#include <utils/utils.h>

#include "constants.h"
#include "hpack_exception.h"
#include "huffman.h"
#include "integer.h"

namespace rfc7541 {

using namespace constants;

ibitstream &ibitstream::operator>>(command &cmd) {
  //    1 - indexed header field (0x80)
  //   01 - header field with incemental indexing (0x40)
  //  001 - change table size (0x20)
  // 0000 - header field without indexing (0x0)
  // 0001 - header field never indexed (0x10)

  check_enought_bytes(1);

  auto byte = data.data()[offset / 8];
  if (byte & cmd_info::get(command::INDEX).value) {
    cmd = command::INDEX;
    offset += cmd_info::get(command::INDEX).bitlen;
  } else if ((byte & cmd_info::get(command::LITERAL_INCREMENTAL_INDEX).mask) ==
             cmd_info::get(command::LITERAL_INCREMENTAL_INDEX).value) {
    cmd = command::LITERAL_INCREMENTAL_INDEX;
    offset += cmd_info::get(command::LITERAL_INCREMENTAL_INDEX).bitlen;
  } else if ((byte & cmd_info::get(command::CHANGE_TABLE_SIZE).value) ==
             cmd_info::get(command::CHANGE_TABLE_SIZE).value) {
    cmd = command::CHANGE_TABLE_SIZE;
    offset += cmd_info::get(command::CHANGE_TABLE_SIZE).bitlen;
  } else if ((byte & cmd_info::get(command::LITERAL_WITHOUT_INDEX).mask) ==
             cmd_info::get(command::LITERAL_WITHOUT_INDEX).value) {
    cmd = command::LITERAL_WITHOUT_INDEX;
    offset += cmd_info::get(command::LITERAL_WITHOUT_INDEX).bitlen;
  } else {
    cmd = command::LITERAL_NEVER_INDEX;
    offset += cmd_info::get(command::LITERAL_NEVER_INDEX).bitlen;
  }
  return *this;
}

ibitstream &ibitstream::operator>>(uint32_t &nr) {
  auto result = integer::decode(offset % 8, data.subspan(offset / 8));
  offset += result.used_bytes * 8;
  offset &= ~utils::make_mask<uint64_t>(3);
  nr = result.value;
  return *this;
}

ibitstream &ibitstream::operator>>(std::vector<uint8_t> &str) {
  check_enought_bytes(1);

  bool is_huffman = data.data()[offset / 8] & constants::ENCODED_STRING_FLAG;
  ++offset;
  uint32_t len = 0;
  (*this) >> len;

  // FIXME: Check len is not greater than allowed data
  check_enought_bytes(len);

  if (!is_huffman) {
    str.assign(data.data() + offset / 8, data.data() + offset / 8 + len);
    offset += len * 8;
  } else {
    str = read_huffman_str(len);
  }

  return *this;
}

uint32_t ibitstream::take_bits(std::size_t len) {
  // NOTE: len <= 32
  auto last_ind = (offset + len) / 8;
  auto last_gap = 8 - (offset + len) % 8;
  auto bytes_len = last_ind - offset / 8 + 1;
  auto first_gap = offset % 8;

  // NOTE: we can read 6 bytes before a real byte data by performance issue
  auto *first = reinterpret_cast<const uint64_t *>(data.data() + last_ind - 7);
  uint64_t bits = boost::endian::big_to_native(*first);

  bits &= utils::make_mask<uint64_t>(len) << last_gap;
  bits <<= (32 - bytes_len * 8 + first_gap);
  return bits;
}

void ibitstream::commit_bits(std::size_t bit_len) {
  check_enought_bits(bit_len);
  offset += bit_len;
}

std::vector<uint8_t> ibitstream::read_huffman_str(std::size_t len) {
  std::vector<uint8_t> result;

  // FIXME: len is an  encoded byte size.
  // The real size can be greater
  result.reserve(len + len / 2);

  uint32_t hcode;
  const size_t end_offset = offset + 8 * len;
  auto codeLengths = rfc7541::huffman::allowed_code_lengths();

  do {
    std::optional<rfc7541::huffman::value_type> index;
    uint8_t code_len = 0;
    for (auto bit_len : codeLengths) {
      // Check EOS padding
      if (end_offset - offset < 8) {
        auto mask = utils::make_mask<uint8_t>(end_offset - offset);
        if (mask == (data.data()[offset / 8] & mask)) {
          offset = end_offset;
          return result;
        }
      }

      hcode = take_bits(bit_len);
      index = rfc7541::huffman::decode({hcode, bit_len});
      if (index) {
        code_len = bit_len;
        break;
      }
    }

    if (!index || code_len == 0) {
      throw hpack_exception("Can't decode huffman code", offset);
    }

    commit_bits(code_len);

    // FIXME: Check EOS. Do  we really need it here?
    if (index.value() == rfc7541::huffman::EOS) {
      break;
    }

    result.push_back(index.value());
  } while (offset < end_offset);

  return result;
}

void ibitstream::check_enought_bytes(std::size_t bytes) const { check_enought_bits(bytes * 8); }

void ibitstream::check_enought_bits(std::size_t bites) const {
  std::size_t data_size = data.size_bytes() * 8;
  std::size_t required_size = offset + bites;

  if (required_size > data_size) {
    throw hpack_exception("Not enought data", offset);
  }
}

} // namespace rfc7541
