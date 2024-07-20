#include <iostream>
#include <thread>

#include <boost/asio/io_context.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/url.hpp>

#include <client_session.h>
#include <connection.h>

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
                std::cerr << "HTTP2 Client unexpected stopped: " << ec << std::endl;
              }
            },
            boost::asio::use_future)
        .get();

    http2::request request(argv[1], http2::method::GET);
    request.headers({{"accept", "*/*"}, {"user-agent", "h2pp/0.0.1"}});
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
