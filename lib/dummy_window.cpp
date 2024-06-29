#include "dummy_window.h"

#include "frame_builder.h"

namespace http2 {

bool dummy_window::need_update() const {
  auto inc_size = max_size - size;
  return inc_size >= trigger_size;
}

std::optional<utils::buffer> dummy_window::update(boost::endian::big_uint32_t http_id) {
  if (!need_update()) {
    return std::nullopt;
  }
  auto inc_size = max_size - size;
  size += inc_size;
  return frame_builder::update_window(inc_size, http_id);
}

} // namespace http2
