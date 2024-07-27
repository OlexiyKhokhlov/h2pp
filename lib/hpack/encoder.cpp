#include "encoder.h"

#include <boost/endian/conversion.hpp>

#include "encoder_stream.h"
#include "huffman.h"
#include "integer.h"
#include "utils/utils.h"

using namespace rfc7541;

namespace rfc7541 {

encoder::encoder() : table(config.init_table_size) {}

std::pair<std::deque<utils::buffer>, std::size_t> encoder::encode(std::deque<rfc7541::header_field> fields,
                                                                  std::size_t size_limit) {
  encoder_stream out(size_limit);

  std::size_t encoded_fields = 0;
  std::size_t bytes_left = size_limit;

  for (const auto &f : fields) {
    uint32_t field_size = 0;

    // TODO: do not lookup value when it isn't required!
    auto index = table.field_index(f.name(), f.value());
    if (f.type() != index_type::DEFAULT) {
      index.second = false;
    }

    if (index.first != -1 && index.second) {
      // Fully indexed. Use command::INDEX

      auto encoded_index = integer::encode(command::INDEX, index.first);
      field_size += encoded_index.length;
      if (field_size > bytes_left) {
        break;
      }
      out.push_back(encoded_index.as_span());

      bytes_left -= field_size;
      ++encoded_fields;

      continue;
    }

    command cmd = command::LITERAL_INCREMENTAL_INDEX;
    if (f.type() != index_type::DEFAULT) {
      cmd = f.type() == index_type::WITHOUT_INDEX ? command::LITERAL_WITHOUT_INDEX : command::LITERAL_NEVER_INDEX;
    }

    if (index.first != -1) {
      // Any type of LITERAL where name is already indexed

      // TODO: do not put in the table to huge literals!

      // check size by name index & cmd
      auto name_index = integer::encode(cmd, index.first);
      field_size += name_index.length;
      if (field_size > bytes_left) {
        break;
      }

      // Check size with value string
      auto value_sz = estimate_string_size(f.value());
      field_size += value_sz.first;
      if (field_size > bytes_left) {
        break;
      }

      // Check size with value length
      auto encoded_value_size = integer::encode(value_sz.second, value_sz.first);
      field_size += encoded_value_size.length;
      if (field_size > bytes_left) {
        break;
      }

      out.push_back(name_index.as_span());
      out.write_string(value_sz, encoded_value_size, f.value());

      if (cmd == command::LITERAL_INCREMENTAL_INDEX) {
        table.insert(f.name(), f.value());
      }

      bytes_left -= field_size;
      ++encoded_fields;

      continue;
    }

    // And the last situation when name and value aren't indexed
    // TODO: do not put in the table to huge literals!

    auto name_sz = estimate_string_size(f.name());
    field_size += name_sz.first;
    if (field_size > bytes_left) {
      break;
    }
    auto encoded_name_size = integer::encode(name_sz.second, name_sz.first);
    field_size += encoded_name_size.length;
    if (field_size > bytes_left) {
      break;
    }

    auto value_sz = estimate_string_size(f.value());
    field_size += value_sz.first;
    if (field_size > bytes_left) {
      break;
    }
    auto encoded_value_size = integer::encode(value_sz.second, value_sz.first);
    field_size += encoded_value_size.length;
    if (field_size > bytes_left) {
      break;
    }

    auto v = cmd_info::get(cmd).value;
    out.push_back({&v, 1});
    out.write_string(name_sz, encoded_name_size, f.name());
    out.write_string(value_sz, encoded_value_size, f.value());

    bytes_left -= field_size;
    ++encoded_fields;
  }

  return {out.flush(), encoded_fields};
}

std::pair<uint32_t, constants::string_flag> encoder::estimate_string_size(std::span<const uint8_t> src) {
  auto encoded_bit_len = huffman::estimate_len(src);
  // At this point a string size must be less that 2^24-1
  uint32_t encoded_bytes_len = static_cast<uint32_t>(utils::ceil_order2(encoded_bit_len, 3) / 8);
  auto src_len = src.size();
  if (encoded_bytes_len < src_len && (src_len < 10 || (100 * encoded_bytes_len / src_len) <= config.min_huffman_rate)) {
    // Use huffman codes
    return {encoded_bytes_len, constants::string_flag::ENCODED};
  }
  // Use as is
  return {src_len, constants::string_flag::INPLACE};
}

} // namespace rfc7541
