[![Support Ukraine](https://img.shields.io/badge/Support-Ukraine-FFD500?style=flat&labelColor=005BBB)](https://opensource.facebook.com/support-ukraine)
[![Build and Test](https://github.com/OlexiyKhokhlov/h2pp/actions/workflows/build.yml/badge.svg)](https://github.com/OlexiyKhokhlov/h2pp/actions/workflows/build.yml)

# h2pp - modern C++ HTTP/2 client library

This is an implementation of HTTP/2 in C++20 that works over [boost asio](https://www.boost.org/doc/libs/1_84_0/doc/html/boost_asio.html)

The main goal of the library is provide a simple and usefull HTTP/2 client.

## Example

This is a simple command line application that sends a GET request by the given url.
A full buildable sample is [here](https://github.com/OlexiyKhokhlov/h2pp/tree/main/examples)

```C++
#include <iostream>
#include <thread>

#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/url.hpp>

#include <client_session.h>

boost::asio::io_context io_context;

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: https_client <url>" << std::endl;
    return EXIT_FAILURE;
  }

  http2::client_session<http2::connection<>> client(io_context);

  std::jthread iothr([]() {
    boost::asio::io_context::work work(io_context);
    io_context.run();
  });

  try {
    auto url = boost::urls::parse_uri(argv[1]);
    if (url.has_error()) {
      return EXIT_FAILURE;
    }

    std::string service = url->port();
    if (service.empty()) {
      service = "443";
    }

    client
        .async_connect(
            url->host_name(), service,
            [](boost::system::error_code ec) {
              if (ec) {
                std::cerr << "HTTP2 Client unexpectedly stopped: " << ec << std::endl;
              }
            },
            boost::asio::use_future)
        .get();

    http2::request request;
    request.method(http2::method::GET).url(argv[1]).headers({{"accept", "*/*"}, {"user-agent", "h2pp/0.0.1"}});
    auto response = client.async_send(std::move(request), boost::asio::use_future).get();
    for (const auto &h : response.headers()) {
      std::cout << h.name_view() << ":  " << h.value_view() << std::endl;
    }
    std::cout << response.body_as<std::string>() << std::endl;

    client.async_disconnect(boost::asio::use_future).get();
  } catch (std::exception &e) {
    std::cerr << "Exception: " << e.what() << std::endl;
  }

  io_context.stop();
  iothr.join();

  return EXIT_SUCCESS;
}

```

## Dependencies

- [Boost asio, url, endiannes](https://www.boost.org/);
- [OpenSSL]( https://www.openssl.org/). An experienced user can use any other equivalent but an integration on you;

## Development status

I've fixed al known bugs so it works...
But it isn't so well tested as required to say - release.
Also is not implemented:
 - pushes;
 - stream priorities;
 - dynamic settings changing;

## License

Distributed under the [GNU Aferro General Public License  v3.0](https://www.gnu.org/licenses/agpl-3.0.en.html#license-text)

If need more just ask me. Everything is negotiable.

## Usefull links

- [HTTP/2 Specifications](https://http2.github.io/)
- [nghttp2 - well known HTTP/2 library in C](https://github.com/nghttp2/nghttp2)
- [Proxygen: Facebook's C++ HTTP Libraries](https://github.com/facebook/proxygen)
