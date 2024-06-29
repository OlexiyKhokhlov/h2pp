#pragma once

#include <deque>

#include "buffer.h"

namespace utils {

/**
 * @brief The streambuf class is infinite buffer that can save and save a data.
 * Internally it holds 'std::deque<buffer>'
 * The minimal buffer size is equal to cache line size.
 * The maximal buffer size is multiple of page size.
 * All stored data cab  allowed via 'flush' method
 */
class streambuf {
public:
  streambuf() = default;
  streambuf(const streambuf &) = delete;
  streambuf &operator=(const streambuf &) = delete;
  streambuf(streambuf &&) = default;
  streambuf &operator=(streambuf &&) = default;
  ~streambuf();

  /**
   * @brief push_back saves a data by the give span into the streambuf
   * @param span
   */
  void push_back(std::span<const uint8_t> src);

  /**
   * @brief flush flushes all stored data into std::deque<buffer> and returns it
   * and cleares all internall data;
   * @return
   */
  std::deque<buffer> flush();

private:
  void check_size(const size_t sz);

private:
  std::deque<buffer> buffers;
};

} // namespace utils
