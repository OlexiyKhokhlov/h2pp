#include "streambuf.h"

#include <deque>

#include "buffer.h"
#include "utils.h"

namespace utils {

namespace {
constexpr size_t MinBufferSize = 64;
constexpr size_t MaxBufferSize = 4096;

std::size_t align_size(std::size_t size) {
  if (size > MaxBufferSize) {
    return ceil_order2(size, 12); // 2^12 == 4096
  }

  return ceil_order2(size, 6); // 2^6 == 64
}

} // namespace

streambuf::~streambuf() = default;

void streambuf::push_back(std::span<const uint8_t> src) {
  auto count = src.size_bytes();
  size_t offset = 0;
  while (count > 0) {
    check_size(count);

    auto dst = buffers.back().prepare();
    auto n = std::min(count, dst.size_bytes());
    memcpy(dst.data(), src.data() + offset, n);
    buffers.back().commit(n);
    count -= n;
    offset += n;
  }
}

std::deque<buffer> streambuf::flush() {
  std::deque<buffer> ret;
  ret.swap(buffers);
  return ret;
}

void streambuf::check_size(const size_t sz) {
  if (buffers.empty()) {
    auto aligned_size = align_size(sz);
    auto size = std::max(aligned_size, MinBufferSize);
    buffers.push_back(buffer(size));
    return;
  }

  auto &last_buffer = buffers.back();
  if (last_buffer.prepare().empty()) {
    auto next_size = std::min(last_buffer.max_size() * 2, MaxBufferSize);
    auto aligned_size = align_size(sz);
    auto size = std::max(aligned_size, next_size);
    buffers.push_back(buffer(size));
    return;
  }
}

} // namespace utils
