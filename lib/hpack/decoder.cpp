#include "decoder.h"

#include <functional>

#include "bitstream.h"
#include "hpack_exception.h"

namespace rfc7541 {

using namespace constants;

header decoder::decode(std::span<const uint8_t> data) {
  using cmd_ptr = void (decoder::*)(ibitstream &stream, header &);
  static cmd_ptr commands[] = {
      &decoder::index_cmd,
      &decoder::literal_incremental_index_cmd,
      &decoder::change_table_size_cmd,
      &decoder::literal_without_index_cmd,
      &decoder::literal_never_index_cmd,
  };

  ibitstream stream(data);
  header h;
  command cmd;
  while (!stream.is_finished()) {
    stream >> cmd;
    std::invoke(commands[static_cast<unsigned>(cmd)], this, stream, h);
  }
  return h;
}

void decoder::index_cmd(ibitstream &stream, header &header) {
  std::size_t index = 0;
  stream >> index;
  if (index == 0) {
    throw hpack_exception("Invalid index value", stream.bit_pos());
  }
  const auto [name, value] = table.at(index);
  header.emplace_back(name, value);
}

void decoder::change_table_size_cmd(ibitstream &stream, header & /*h*/) {
  std::size_t max_size = 0;
  stream >> max_size;
  table.update_size(max_size);
}

void decoder::literal_without_index_cmd(ibitstream &stream, header &h) {
  literal_without_index_impl(stream, h, index_type::WITHOUT_INDEX);
}

void decoder::literal_never_index_cmd(ibitstream &stream, header &h) {
  literal_without_index_impl(stream, h, index_type::NEVER_INDEX);
}

void decoder::literal_without_index_impl(ibitstream &stream, header &h, index_type type) {
  std::size_t index = 0;
  stream >> index;

  header_field field(type);
  if (index == 0) {
    std::vector<uint8_t> name;
    stream >> name;
    field.set_name(std::move(name));
  } else {
    const auto [name, value] = table.at(index);
    std::vector<uint8_t> name_val(name.begin(), name.end());
    field.set_name(std::move(name_val));
  }
  std::vector<uint8_t> value;
  stream >> value;
  field.set_value(std::move(value));
  h.emplace_back(field);
}

void decoder::literal_incremental_index_cmd(ibitstream &stream, header &h) {
  literal_without_index_impl(stream, h, index_type::DEFAULT);
  table.insert(h.back().name(), h.back().value());
}
} // namespace rfc7541