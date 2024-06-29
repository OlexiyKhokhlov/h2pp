#pragma once

#include <stdexcept>

class hpack_exception : public std::runtime_error {
public:
  hpack_exception(const std::string &what_arg, std::size_t offset = 0);
  hpack_exception(const char *what_arg, std::size_t offset = 0);
  hpack_exception(const hpack_exception &other) noexcept;

  std::size_t offset() const { return bit_offset; }

private:
  std::size_t bit_offset = 0;
};

/**
  Decoder errors:
    - wrong symbol in name;
    - wrong command value;
    - wrong index (0 or bigger that table);
    - wrong string length (not enought data in frame);
    - not enought bytes general;
    - wrong huffman code;
    - an empty name?

Encoder errors:
 - wrong symbol in name;
 - value is too long(longer than frame payload);
 - an empty name?
*/
