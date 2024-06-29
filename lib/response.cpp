#include "response.h"

#include <algorithm>

#include "frame.h"
#include "protocol.h"

namespace http2 {

void response::insert_headers(rfc7541::header &&header) {
  std::move(header.begin(), header.end(), std::back_inserter(header_list));
  if (status_view.empty()) {
    if (auto it = std::find_if(header_list.begin(), header_list.end(),
                               [](const auto &hf) { return hf.name_view() == ":status"; });
        it != header_list.end()) {
      status_view = it->value_view();
    }
  }
}

void response::insert_body(utils::buffer &&b) {
  const auto analyzer = frame_analyzer::from_buffer(b.data_view());
  auto span = analyzer.template get_frame<frame_type::DATA>().data();
  body_blocks.emplace_back(std::move(b), span);
  size += span.size_bytes();
}

std::size_t response::copy_body(char *dst, std::size_t len) const {
  std::size_t offset = 0;
  for (const auto &s : body_blocks) {
    if (offset == len) {
      return offset;
    }
    auto to_copy = std::min(s.span.size_bytes(), len - offset);
    memcpy(dst + offset, s.span.data(), to_copy);
    offset += to_copy;
  }
  return offset;
}

} // namespace http2
