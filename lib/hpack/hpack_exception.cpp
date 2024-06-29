#include "hpack_exception.h"

hpack_exception::hpack_exception(const std::string &what_arg, std::size_t offset)
    : std::runtime_error(what_arg), bit_offset(offset) {}

hpack_exception::hpack_exception(const char *what_arg, std::size_t offset)
    : std::runtime_error(what_arg), bit_offset(offset) {}

hpack_exception::hpack_exception(const hpack_exception &other) noexcept
    : std::runtime_error(other), bit_offset(other.bit_offset) {}
