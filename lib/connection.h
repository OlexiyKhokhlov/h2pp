#pragma once

#include <cstdint>
#include <memory>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/ssl.hpp>


namespace http2 {

template <typename NetworkType = boost::asio::ip::tcp> class connection {
public:
  using SocketType = typename boost::asio::ssl::stream<typename NetworkType::socket>;

  connection(boost::asio::io_context &io)
      : ssl_context(boost::asio::ssl::context::sslv23), ssl_socket(io, ssl_context) {}

  connection(connection &&c) : ssl_context(std::move(c.ssl_context)), ssl_socket(std::move(c.ssl_socket)) {}

  connection(const connection &c) = delete;
  virtual ~connection() = default;

  template <typename CompletionToken>
  auto async_connect(std::string_view host, std::string_view service, CompletionToken &&token);

  template <typename CompletionToken> auto async_disconnect(CompletionToken &&token) {
    boost::system::error_code ec;
    // TODO: Probably have to call ssl_socket.async_shutdown with timeout?
    ssl_socket.lowest_layer().close(ec);
    return ssl_socket.async_shutdown(std::move(token));
  }

  // template<typename MutableBufferSequence, typename CompletionToken>
  // auto async_read_some(const MutableBufferSequence &buffers,
  // CompletionToken &&token)
  // {
  //     return ssl_socket.async_read_some(buffers, std::move(token));
  // }

  template <typename MutableBufferSequence, typename CompletionToken>
  auto async_read(const MutableBufferSequence &buffers, CompletionToken &&token) {
    return boost::asio::async_read(ssl_socket, buffers, boost::asio::transfer_at_least(1), token);
  }

  // template<typename ConstBufferSequence, typename CompletionToken>
  // auto async_write_some(const ConstBufferSequence &buffers, CompletionToken
  // &&token)
  // {
  //     return ssl_socket.async_write_some(buffers, std::move(token));
  // }

  template <typename ConstBufferSequence, typename CompletionToken>
  auto async_write(const ConstBufferSequence &buffers, CompletionToken &&token) {
    return boost::asio::async_write(ssl_socket, buffers, token);
  }

protected:
  virtual void prepare_ssl(std::string_view host);
  virtual bool verify_certificate(bool preverified, boost::asio::ssl::verify_context &ctx);

private:
  boost::asio::awaitable<void> co_connect(std::string_view host, std::string_view service);

private:
  boost::asio::ssl::context ssl_context;
  SocketType ssl_socket;
};

template <typename NetworkType>
bool connection<NetworkType>::verify_certificate(bool /*preverified*/, boost::asio::ssl::verify_context &ctx) {
  // The verify callback can be used to check whether the certificate that is
  // being presented is valid for the peer. For example, RFC 2818 describes
  // the steps involved in doing this for HTTPS. Consult the OpenSSL
  // documentation for more details. Note that the callback is called once
  // for each certificate in the certificate chain, starting from the root
  // certificate authority.

  // In this example we will simply print the certificate's subject name.
  char subject_name[256];
  X509 *cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
  X509_NAME_oneline(X509_get_subject_name(cert), subject_name, std::size(subject_name));

  // return preverified;
  return true;
}

template <typename NetworkType> void connection<NetworkType>::prepare_ssl(std::string_view host) {
  ssl_context.set_default_verify_paths();
  static char HTTP2_PROTO[] = {2, 'h', '2'};

  if (0 != SSL_set_alpn_protos(ssl_socket.native_handle(), reinterpret_cast<const unsigned char *>(HTTP2_PROTO),
                               std::size(HTTP2_PROTO))) {
    boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
    throw boost::system::system_error{ec};
  }

  std::string host_str(host);
  if (0 == SSL_set_tlsext_host_name(ssl_socket.native_handle(), host_str.c_str())) {
    boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
    throw boost::system::system_error{ec};
  }

  ssl_socket.set_verify_mode(boost::asio::ssl::verify_peer);
  ssl_socket.set_verify_callback(
      [this](bool preverified, boost::asio::ssl::verify_context &ctx) { return verify_certificate(preverified, ctx); });
}

template <typename NetworkType>
boost::asio::awaitable<void> connection<NetworkType>::co_connect(std::string_view host, std::string_view service) {
  typename NetworkType::resolver resolver{ssl_socket.get_executor()};
  auto endpoints = co_await resolver.async_resolve(host, service, boost::asio::use_awaitable);
  auto epoint = co_await boost::asio::async_connect(ssl_socket.lowest_layer(), endpoints, boost::asio::use_awaitable);

  co_await ssl_socket.async_handshake(boost::asio::ssl::stream_base::client, boost::asio::use_awaitable);

  co_return;
}

template <typename NetworkType>
template <typename CompletionToken>
auto connection<NetworkType>::async_connect(std::string_view host, std::string_view service, CompletionToken &&token) {
  ssl_context = boost::asio::ssl::context{boost::asio::ssl::context::sslv23};
  ssl_socket = SocketType{ssl_socket.get_executor(), ssl_context};

  prepare_ssl(host);

  return boost::asio::co_spawn(ssl_socket.get_executor(), co_connect(host, service), token);
}
} // namespace http2
