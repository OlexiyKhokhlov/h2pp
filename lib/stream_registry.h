#pragma once

#include <deque>
#include <map>
#include <mutex>

#include "utils/buffer.h"

#include "stream.h"

namespace rfc7541 {
class encoder;
}

namespace http2 {

class stream_registry {
public:
  stream_registry() = default;
  stream_registry(stream_registry &&) = delete;
  stream_registry(const stream_registry &) = delete;
  ~stream_registry();

  void add_stream(boost::endian::big_uint32_t id, stream::ptr sptr);
  stream::ptr get_stream(boost::endian::big_uint32_t id);

  void enqueue(stream::ptr);
  void erase(boost::endian::big_uint32_t id);

  std::size_t get_data(std::deque<utils::buffer> &out, rfc7541::encoder &enc, std::size_t limit);
  void reset(const boost::system::error_code &ec);

private:
  bool has_data() const;
  stream::ptr scheduled_stream();

private:
  std::mutex mutex;
  std::map<boost::endian::big_uint32_t, stream::ptr> stream_map;
  std::deque<stream::ptr> queue;
};

} // namespace http2
