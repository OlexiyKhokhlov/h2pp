#pragma once

#include <cstdint>
#include <optional>

#include <boost/endian.hpp>

#include <utils/buffer.h>

namespace http2 {

class dummy_window {
public:
  explicit dummy_window(uint32_t msize, uint32_t trigger = 1024)
      : max_size(msize), size(msize), trigger_size(trigger) {}

  void dec(uint32_t v) { size -= v; }
  bool need_update() const;
  std::optional<utils::buffer> update(boost::endian::big_uint32_t http_id);
  uint32_t current() const { return size; }

private:
  uint32_t max_size;
  uint32_t size;
  uint32_t trigger_size;
};

} // namespace http2
