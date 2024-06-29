#pragma once

#include <mutex>
#include <optional>

#include <boost/asio/any_completion_handler.hpp>
#include <boost/asio/steady_timer.hpp>

#include "protocol.h"
#include "utils/buffer.h"

namespace http2 {
class settings_manager {
public:
  explicit settings_manager(boost::asio::io_context &context);
  settings_manager(const settings_manager &) = delete;
  settings_manager(settings_manager &&) = delete;
  ~settings_manager();

  http2::settings &get_local_settings() { return local_settings; }
  http2::settings &get_server_settings() { return remote_settings; }

  utils::buffer initiate_sync_settings(settings &&local, bool remote_sync,
                                       boost::asio::any_completion_handler<void(boost::system::error_code)> &&);
  bool cancel(const boost::system::error_code &ec);

  std::optional<utils::buffer> on_settings_frame(std::span<const uint8_t>);

private:
  bool finished(const boost::system::error_code &ec);

private:
  http2::settings local_settings;
  http2::settings remote_settings;

  bool need_remote_sync = true;
  bool local_settings_ack = false;
  bool remote_settings_got = false;
  boost::asio::steady_timer settings_timer;
  std::mutex mutex;
  boost::asio::any_completion_handler<void(boost::system::error_code)> settings_handler;
};
} // namespace http2
