#include "decoder.h"

#include <functional>

#include "constants.h"
#include "integer.h"
#include "string.h"

namespace rfc7541 {

using namespace constants;

namespace {
command decode_cmd(uint8_t byte) {
  if (byte & cmd_info::get(command::INDEX).value) {
    return command::INDEX;
  } else if ((byte & cmd_info::get(command::LITERAL_INCREMENTAL_INDEX).mask) ==
             cmd_info::get(command::LITERAL_INCREMENTAL_INDEX).value) {
    return command::LITERAL_INCREMENTAL_INDEX;
  } else if ((byte & cmd_info::get(command::CHANGE_TABLE_SIZE).value) ==
             cmd_info::get(command::CHANGE_TABLE_SIZE).value) {
    return command::CHANGE_TABLE_SIZE;
  } else if ((byte & cmd_info::get(command::LITERAL_WITHOUT_INDEX).mask) ==
             cmd_info::get(command::LITERAL_WITHOUT_INDEX).value) {
    return command::LITERAL_WITHOUT_INDEX;
  } else {
    return command::LITERAL_NEVER_INDEX;
  }
}
} // namespace

decoder::~decoder() = default;

header decoder::decode(std::span<const uint8_t> src) {
  // using cmd_ptr = void (decoder::*)(ibitstream &stream, header &);
  using cmd_ptr = std::span<const uint8_t> (decoder::*)(std::span<const uint8_t>, header &);
  static cmd_ptr commands[] = {
      &decoder::index_cmd,
      &decoder::literal_incremental_index_cmd,
      &decoder::change_table_size_cmd,
      &decoder::literal_without_index_cmd,
      &decoder::literal_never_index_cmd,
  };

  header fields;

  while (!src.empty()) {
    auto cmd = decode_cmd(src.front());
    src = std::invoke(commands[static_cast<unsigned>(cmd)], this, src, fields);
  }

  return fields;
}

std::span<const uint8_t> decoder::index_cmd(std::span<const uint8_t> src, header &header) {
  auto index = integer::decode(cmd_info::get(command::INDEX).bitlen, src);
  if (index.value == 0) {
    throw std::runtime_error("Invalid index value");
  }
  const auto [name, value] = table.at(index.value);
  header.emplace_back(name, value);
  return src.subspan(index.used_bytes);
}

std::span<const uint8_t> decoder::change_table_size_cmd(std::span<const uint8_t> src, header & /*h*/) {
  auto max_size = integer::decode(cmd_info::get(command::CHANGE_TABLE_SIZE).bitlen, src);
  table.update_size(max_size.value);
  return src.subspan(max_size.used_bytes);
}

std::span<const uint8_t> decoder::literal_without_index_cmd(std::span<const uint8_t> src, header &h) {
  return literal_without_index_impl(src, h, index_type::WITHOUT_INDEX,
                                    cmd_info::get(command::LITERAL_WITHOUT_INDEX).bitlen);
}

std::span<const uint8_t> decoder::literal_never_index_cmd(std::span<const uint8_t> src, header &h) {
  return literal_without_index_impl(src, h, index_type::NEVER_INDEX,
                                    cmd_info::get(command::LITERAL_NEVER_INDEX).bitlen);
}

std::span<const uint8_t> decoder::literal_incremental_index_cmd(std::span<const uint8_t> src, header &h) {
  auto res =
      literal_without_index_impl(src, h, index_type::DEFAULT, cmd_info::get(command::LITERAL_INCREMENTAL_INDEX).bitlen);
  table.insert(h.back().name(), h.back().value());
  return res;
}

std::span<const uint8_t> decoder::literal_without_index_impl(std::span<const uint8_t> src, header &h, index_type type,
                                                             uint8_t bits) {
  auto index = integer::decode(bits, src);
  src = src.subspan(index.used_bytes);

  std::vector<uint8_t> name;
  if (index.value == 0) {
    auto decoded_name = string::decode(src);
    src = src.subspan(decoded_name.used_bytes);
    name = std::move(decoded_name.value);
  } else {
    const auto [name_view, value_view] = table.at(index.value);
    std::ignore = value_view;
    name = std::vector<uint8_t>(name_view.begin(), name_view.end());
  }

  std::vector<uint8_t> value;
  auto decoded_value = string::decode(src);
  src = src.subspan(decoded_value.used_bytes);
  h.emplace_back(std::move(name), std::move(decoded_value.value), type);
  return src;
}

} // namespace rfc7541
