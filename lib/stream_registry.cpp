#include "stream_registry.h"

#include "hpack/encoder.h"

namespace http2 {
stream_registry::~stream_registry() = default;

void stream_registry::add_stream(boost::endian::big_uint32_t id, stream::ptr sptr) {
  std::scoped_lock lock(mutex);

  /*auto [it, inserted] =*/stream_map.emplace(id, sptr);
  queue.push_back(sptr);
}

stream::ptr stream_registry::get_stream(boost::endian::big_uint32_t id) {
  std::scoped_lock lock(mutex);
  if (auto it = stream_map.find(id); it != stream_map.end()) {
    return it->second;
  }
  return {};
}

bool stream_registry::has_data() const { return !queue.empty(); }

void stream_registry::enqueue(stream::ptr stream) {
  std::scoped_lock lock(mutex);
  queue.push_back(stream);
}

void stream_registry::erase(boost::endian::big_uint32_t id) {
  std::scoped_lock lock(mutex);

  if (auto it = std::find_if(queue.begin(), queue.end(), [id](const auto &ptr) { return id == ptr->id(); });
      it != queue.end()) {
    it->reset();
  }

  stream_map.erase(id);
}

stream::ptr stream_registry::scheduled_stream() {
  std::scoped_lock lock(mutex);

  while (!queue.empty()) {
    auto ret = queue.front();
    queue.pop_front();
    if (ret) {
      return ret;
    }
  }
  return {};
}

std::size_t stream_registry::get_data(std::deque<utils::buffer> &out, rfc7541::encoder &enc, std::size_t limit) {
  decltype(queue) next_queue;

  std::size_t bytes_used = 0;
  do {
    if (limit < 16) {
      // FIXME: find out a smarter criteria
      break;
    }
    auto stream = scheduled_stream();
    if (!stream) {
      break;
    }

    if (!stream->has_tx_data()) {
      continue;
    }

    bytes_used += stream->get_tx_data(out, enc, limit - bytes_used);
    if (stream->check_tx_data()) {
      next_queue.emplace_back(stream);
    }
  } while (true);

  if (!next_queue.empty()) {
    std::scoped_lock lock(mutex);
    std::move(next_queue.begin(), next_queue.end(), std::back_inserter(queue));
  }

  return bytes_used;
}

void stream_registry::reset(const boost::system::error_code &ec) {
  std::scoped_lock lock(mutex);

  queue.clear();

  for (auto &stream : stream_map) {
    stream.second->reset(ec);
  }
  stream_map.clear();
}

} // namespace http2
