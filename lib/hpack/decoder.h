#pragma once
;
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
  decoder &operator=(const decoder &) = delete;
  decoder(decoder &&) = delete;
  decoder &operator=(decoder &&) = delete;
  ~decoder();

  header decode(std::span<const uint8_t> encoded_data);

private:
  constexpr static inline size_t DefaultTableSize = 4096;
  decoder_table table{DefaultTableSize};

  std::span<const uint8_t> index_cmd(std::span<const uint8_t> src, header &h);
  std::span<const uint8_t> change_table_size_cmd(std::span<const uint8_t> src, header &h);
  std::span<const uint8_t> literal_without_index_cmd(std::span<const uint8_t> src, header &h);
  std::span<const uint8_t> literal_never_index_cmd(std::span<const uint8_t> src, header &h);
  std::span<const uint8_t> literal_without_index_impl(std::span<const uint8_t> src, header &h, index_type type,
                                                      uint8_t bits);
  std::span<const uint8_t> literal_incremental_index_cmd(std::span<const uint8_t> src, header &h);
};

} // namespace rfc7541
