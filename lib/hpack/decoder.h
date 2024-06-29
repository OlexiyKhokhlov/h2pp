#pragma once

#include <cstdint>
#include <span>

#include "header_field.h"
#include "hpack_table.h"

namespace rfc7541 {

class ibitstream;

class decoder {
public:
  decoder() = default;
  decoder(const decoder &) = delete;
  decoder(decoder &&) = default;
  ~decoder() = default;

  header decode(std::span<const uint8_t> encoded_data);

private:
  decoder_table table{4096};

  void index_cmd(ibitstream &stream, header &h);
  void change_table_size_cmd(ibitstream &stream, header &h);
  void literal_without_index_cmd(ibitstream &stream, header &h);
  void literal_never_index_cmd(ibitstream &stream, header &h);
  void literal_without_index_impl(ibitstream &stream, header &h, index_type type);
  void literal_incremental_index_cmd(ibitstream &stream, header &h);
};

} // namespace rfc7541
