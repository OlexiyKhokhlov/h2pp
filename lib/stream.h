#pragma once

#include <boost/asio/any_completion_handler.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/endian.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include "dummy_window.h"
#include "error.h"
#include "request.h"
#include "response.h"
#include "utils/buffer.h"

namespace rfc7541 {
class encoder;
}

namespace http2 {

class base_client;

/**
 * @brief The stream class represents a HTTP2 stream.
 */
class stream : public boost::intrusive_ref_counter<stream> {
public:
  using ptr = boost::intrusive_ptr<stream>;

  explicit stream(boost::asio::io_context &io, boost::endian::big_uint32_t id, std::size_t remote_size,
                  std::size_t local_size, request &&,
                  boost::asio::any_completion_handler<void(boost::system::error_code, response &&)> &&handler);
  stream() = delete;
  stream(const stream &) = delete;
  stream(stream &&) = delete;
  ~stream();

  request &get_request() { return m_request; }

  boost::endian::big_uint32_t id() const { return http_id; }
  bool has_tx_data() const;
  bool check_tx_data();

  bool is_finished() const { return http_state == HttpState::CLOSED; }
  void reset(const boost::system::error_code &ec);

  // Internal IO
  void on_receive_data(utils::buffer &&buff);
  void on_receive_headers(rfc7541::header &&header, uint8_t flags, std::size_t raw_size);
  void on_receive_reset(error_code err);
  void on_receive_window_update(uint32_t increment);
  void on_receive_continuation(rfc7541::header &&header, uint8_t flags, std::size_t raw_size);

  std::size_t get_tx_data(std::deque<utils::buffer> &out, rfc7541::encoder &enc, std::size_t limit);

private:
  std::size_t prepare_headers(std::deque<utils::buffer> &out, rfc7541::encoder &enc, std::size_t limit);
  std::size_t prepare_body(std::deque<utils::buffer> &out, std::size_t limit);
  void finished(const boost::system::error_code &ec);

private:
  boost::asio::steady_timer timer;
  boost ::endian::big_uint32_t http_id = 0;
  bool is_scheduled = true;
  std::size_t remote_window;
  dummy_window local_window;

  enum class HttpState {
    IDLE,
    OPEN,
    HALF_CLOSED,
    CLOSED,
  };
  HttpState http_state = HttpState::IDLE;
  bool is_cointinuation = false;
  std::size_t send_body_offset = 0;

  request m_request;
  response m_response;
  std::mutex mutex;
  boost::asio::any_completion_handler<void(boost::system::error_code, response &&)> respone_handler;
};

} // namespace http2
