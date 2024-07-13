#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <span>

namespace utils {

/**
 * @brief The buffer class is a wrapper around a fixed sized memory slice.
 * The size is defined at the creation size and can not be changed any more.
 * After the creation the buffer is empty and ready to save a data.
 * A data producer sholuld use methods - 'prepare' and 'commit'
 * A data consumer should use methods - 'data_view' and 'consume'
 * @note technically is possible to create a buffer with 0 bytes size. No exception thrown in this case.
 */
class buffer {
public:
  buffer(const buffer &) = delete;
  buffer &operator=(const buffer &) = delete;
  ~buffer() = default;

  explicit buffer(std::size_t size) : offset(0), maxsize(size), memory(new uint8_t[maxsize]) {}

  buffer(buffer &&rhs) : offset(rhs.offset), maxsize(rhs.maxsize), memory(std::move(rhs.memory)) {
    rhs.maxsize = 0;
    rhs.offset = 0;
  };

  buffer &operator=(buffer &&rhs) {
    memory = std::move(rhs.memory);
    maxsize = rhs.maxsize;
    offset = rhs.offset;
    rhs.maxsize = 0;
    rhs.offset = 0;
    return *this;
  };

  /**
   * @brief prepare
   * @return returns a span of a muttable bytes these are ready to have a new
   * data.
   */
  std::span<uint8_t> prepare() noexcept {
    return {memory.get() + offset, maxsize - offset};
  };

  /**
   * @brief commit should be called after 'prepare' and writing in ti given span.
   * The actual size of writteten bytes is an argument of this method.
   * @param count is count of bytes that has been actually written.
   * @note When count is greater than max allowed size will be used max allowed size instead.
   * @return an actual count of bytes that has been commited
   */
  size_t commit(std::size_t count) noexcept {
    auto delta = std::min(prepare().size_bytes(), count);
    offset += delta;
    return delta;
  }

  /**
   * @brief commit is equal to `commit(std::size_t count)`
   * but commites copies byest from the given span and commites written bytes.
   * @param span is a data that should be commited into buffer.
   * @note is equal to `commit(std::size_t count)` when given span is greater than max allowed size
   *  will copy only max allowed bytes instead
   * @return an actual count of bytes that has been commited
   */
  std::size_t commit(std::span<const uint8_t> span) noexcept {
    auto to_copy = std::min(prepare().size_bytes(), span.size_bytes());
    memcpy(memory.get() + offset, span.data(), to_copy);
    offset += to_copy;
    return to_copy;
  }

  /**
   * @brief data_view should be called by data consumer for retrieving of stored data
   * @return return a span of constant bytes those has bin written.
   */
  std::span<const uint8_t> data_view() const noexcept { return {memory.get(), offset}; }

  /**
   * @brief consume is an auxilary method for consumer.
   * Actually it removes 'count' of the first written bytes.
   * @note usually it is slow since uses memmove for the overlapped regions of memory.
   * So don' recomend to use it everywhere!
   * @param count - is a bytes count that should be removed.
   */
  void consume(std::size_t count) noexcept {
    count = std::min(offset, count);
    if (count != 0 && count < offset) {
      memmove(memory.get(), memory.get() + count, offset - count);
    }
    offset -= count;
  }

  /**
   * @brief max_size
   * @return returns a max size of beuffer. I. e. a size of internal mem size.
   */
  std::size_t max_size() const noexcept { return maxsize; }

private:
  std::size_t offset = 0;
  std::size_t maxsize = 0;
  std::unique_ptr<uint8_t[]> memory;
};

} // namespace utils
