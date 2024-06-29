#pragma once

#include <atomic>
#include <deque>

#include <boost/asio/any_completion_handler.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/endian/arithmetic.hpp>
#include <boost/endian/conversion.hpp>

#include "utils/buffer.h"

#include "request.h"
#include "response.h"

namespace http2 {

/**
 * @brief The base_client class contains HTTP2 client implementation
 * that is not depended on transport code
 */
class base_client {
public:
  explicit base_client(boost::asio::io_context &io);
  base_client(const base_client &) = delete;
  base_client(base_client &&) = delete;
  virtual ~base_client();

protected:
  // Must be implemented in the parent. Is called every time when a base_client
  // wants to send some data
  virtual void init_write() = 0;

  // Async initiation routines
  void initiate_sync_settings(bool remote_sync,
                              boost::asio::any_completion_handler<void(boost::system::error_code)> &&);

  void initiate_disconnect(boost::system::error_code ec, boost::asio::any_completion_handler<void()> &&handler =
                                                             boost::asio::any_completion_handler<void()>{});

  void initiate_send(request &&rq,
                     boost::asio::any_completion_handler<void(boost::system::error_code, response &&)> &&handler);

  void initiate_ping(boost::asio::any_completion_handler<void(boost::system::error_code)> &&handler);

  // RX/TX methods
  utils::buffer on_read(utils::buffer &&);
  void write_initial_frames();
  std::deque<utils::buffer> get_tx_data();
  void cleanup_after_disconnect(const boost::system::error_code &ec);

protected:
  boost::asio::io_context &io;

  // Inition stuff
  std::atomic_flag start_connecting_flag;
  std::atomic_flag is_connected_flag;

  // Disconnecting stuff
  std::atomic_flag start_disconnect_flag;
  boost::system::error_code connection_error_code;
  boost::asio::any_completion_handler<void()> disconnect_handler;

private:
  // RX/TX methods
  void on_receive_frame(std::span<const uint8_t>);
  void on_receive_data_frame(utils::buffer &&);
  void send_command(utils::buffer &&buff);

  void on_receive_data(utils::buffer &&buff);
  void on_receive_data_not_reachable(std::span<const uint8_t>);
  void on_receive_headers(std::span<const uint8_t>);
  void on_receive_priority(std::span<const uint8_t>);
  void on_receive_reset(std::span<const uint8_t>);
  void on_receive_settings(std::span<const uint8_t>);
  void on_receive_push(std::span<const uint8_t>);
  void on_receive_ping(std::span<const uint8_t>);
  void on_receive_goaway(std::span<const uint8_t>);
  void on_receive_window_update(std::span<const uint8_t>);
  void on_receive_continuation(std::span<const uint8_t>);

  static decltype(&base_client::on_receive_headers) frame_handlers[];

  // Should be 1,3,5,...
  boost::endian::big_uint32_t get_next_stream_id() {
    auto order = client_id_order++;
    return boost::endian::native_to_big(order * 2 - 1);
  }

private:
  // Is a base value for outgoing http2 streams
  uint32_t client_id_order = 1;

  struct PrivateClient;
  std::unique_ptr<PrivateClient> private_client;

  // Separate queue for non-stream hi priority frames
  std::deque<utils::buffer> tx_command_queue;
  std::mutex command_queue_mutex;
  std::size_t server_window_size;

  boost::asio::any_completion_handler<void(boost::system::error_code)> ping_handler;
};
} // namespace http2
