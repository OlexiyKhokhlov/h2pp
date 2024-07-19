#pragma once

#include <boost/asio/async_result.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/error.hpp>

#include "base_client.h"

namespace http2 {

template <typename ConnectionType> class client_session : public base_client {
public:
  explicit client_session(boost::asio::io_context &io) : base_client(io), connection(io) {}

  client_session(const client_session &c) = delete;
  client_session(client_session &&c) = delete;
  ~client_session() = default;

  /**
   * @brief async_connect starts connection to remote HTTP2 server.
   * @param host is a host name or ip addr
   * @param service is a port number
   * @param handler is a handler that will be called in the case
   * when this session is closed by server or closed accidentally by network
   * disconnecting. Also it will be called when the disconenction is initiated
   * on the client side.
   * @param token is a completion token with signature
   * 'void(boost::system:error_code)'
   */
  template <typename CompletionToken, typename ShutdownHandler>
  auto async_connect(std::string_view host, std::string_view service, ShutdownHandler &&handler,
                     CompletionToken &&token) {

    shutdown_handler = std::move(handler);
    return boost::asio::co_spawn(io, co_init(host, service), std::forward<CompletionToken>(token));
  }

  /**
   * @brief async_disconnect starts disconenction.
   * @note when disconnection is completed a handler that has been set via
   * async_connect will be called.
   * @param token is a completion token with signature 'void()'.
   * Even when disconnection has an error it will be closed in any way. So no
   * error_code here as well. (@see for details:
   * https://github.com/boostorg/beast/issues/824)
   */
  template <typename CompletionToken> auto async_disconnect(CompletionToken &&token) {
    using HandlerSignature = void();
    using AnyCompletionHandlerT = boost::asio::any_completion_handler<HandlerSignature>;

    auto init = [this](auto &&h) {
      initiate_disconnect(boost::system::error_code{}, AnyCompletionHandlerT(std::move(h)));
    };

    return boost::asio::async_initiate<CompletionToken, HandlerSignature>(std::move(init), token);
  }

  /**
   * @brief async_send starts sending a request.
   * When request is not valid or max count of parallel streams is reached can
   * throw an exception. A response will be returned via completion token.
   * @param request is a request for sending
   * @param token is a completion token with signature  void(const
   * boost::system:error_code&, response&&)
   */
  template <typename CompletionToken> auto async_send(request &&request, CompletionToken &&token) {
    using HandlerSignature = void(boost::system::error_code, response &&);
    using AnyCompletionHandlerT = boost::asio::any_completion_handler<HandlerSignature>;

    request.check_valid();

    auto init = [this](auto &&h, auto &&rq) { initiate_send(std::move(rq), AnyCompletionHandlerT(std::move(h))); };

    return boost::asio::async_initiate<CompletionToken, HandlerSignature>(std::move(init), token, std::move(request));
  }

  template <typename CompletionToken> auto ping(CompletionToken &&token) {
    using HandlerSignature = void(boost::system::error_code);
    using AnyCompletionHandlerT = boost::asio::any_completion_handler<HandlerSignature>;

    auto init = [this](auto &&h) { initiate_ping(AnyCompletionHandlerT(std::move(h))); };

    return boost::asio::async_initiate<CompletionToken, HandlerSignature>(std::move(init), token);
  }

private:
  template <typename CompletionToken> auto async_update_settings(bool remote_sync, CompletionToken &&token) {
    using HandlerSignature = void(boost::system::error_code);
    using AnyCompletionHandlerT = boost::asio::any_completion_handler<HandlerSignature>;

    auto init = [this](auto &&h, auto r_sync) { initiate_sync_settings(r_sync, AnyCompletionHandlerT(std::move(h))); };

    return boost::asio::async_initiate<CompletionToken, HandlerSignature>(std::move(init), token, remote_sync);
  }

  boost::asio::awaitable<boost::system::error_code> co_init(std::string_view host, std::string_view service) {
    if (is_connected_flag.test()) {
      co_return boost::asio::error::already_connected;
    }

    if (start_connecting_flag.test_and_set()) {
      co_return boost::asio::error::in_progress;
    }

    co_await connection.async_connect(host, service, boost::asio::use_awaitable);

    tx_routine_done.clear();
    tx_running_flag.clear();
    connection_error_code = boost::system::error_code{};

    write_initial_frames();
    init_read();

    co_await async_update_settings(true, boost::asio::use_awaitable);

    is_connected_flag.test_and_set();
    connection_error_code.clear();
    co_return boost::system::error_code();
  }

  void init_read() {
    if (!start_disconnect_flag.test()) {
      auto buff = input_buffer.prepare();
      connection.async_read(boost::asio::buffer(buff.data(), buff.size()),
                            [this](const auto &ec, auto bytes_transferred) { on_data_read(ec, bytes_transferred); });
    }
  }

  void on_data_read(const boost::system ::error_code &ec, std::size_t bytes_transferred) {
    if (!ec) {
      input_buffer.commit(bytes_transferred);
      input_buffer = on_read(std::move(input_buffer));
      init_read();
    } else if (!start_disconnect_flag.test_and_set()) {
      connection_error_code = ec;
    }
  }

  void init_write() final {
    if (tx_running_flag.test_and_set()) {
      return;
    }

    io.post([this]() {
      auto tx_queue = get_tx_data();
      if (tx_queue.empty()) {
        tx_running_flag.clear();
        if (start_disconnect_flag.test()) {
          tx_routine_done.test_and_set();
          check_shutdown();
        }
        return;
      }

      std::vector<boost::asio::const_buffer> buffers;
      buffers.reserve(tx_queue.size());
      std::transform(tx_queue.begin(), tx_queue.end(), std::back_insert_iterator(buffers), [](auto &&v) {
        auto sp = v.data_view();
        return boost::asio::buffer(sp.data(), sp.size());
      });
      std::move(tx_queue.begin(), tx_queue.end(), std::back_inserter(tx_queue_sent));
      connection.async_write(std::move(buffers),
                             [this](const auto &ec, auto bytes_transferred) { on_data_write(ec, bytes_transferred); });
    });
  }

  void on_data_write(const boost::system ::error_code &ec, std::size_t /*bytes_transferred*/){
    if (!ec) {
      tx_queue_sent.clear();
      tx_running_flag.clear();
      init_write();
    } else {
      if (!start_disconnect_flag.test_and_set()) {
        connection_error_code = ec;
      }
      tx_running_flag.clear();
      tx_routine_done.test_and_set();
      return;
    }
  }

  void check_shutdown() {
    if (tx_routine_done.test()) {
      connection.async_disconnect([this](const auto & /*ec*/) {
        cleanup_after_disconnect(connection_error_code);

        if (shutdown_handler) {
          shutdown_handler(connection_error_code);
        }
        if (disconnect_handler) {
          disconnect_handler();
        }
      });
    }
  }

private:
  ConnectionType connection;

  // RX
  utils::buffer input_buffer = utils::buffer(64);

  // TX
  std::atomic_flag tx_running_flag;
  std::atomic_flag tx_routine_done;
  // temporarely keeps a data that is under async_write
  std::deque<utils::buffer> tx_queue_sent;

  // Is called on disconnect
  std::function<void(boost::system::error_code)> shutdown_handler;
};

} // namespace http2
