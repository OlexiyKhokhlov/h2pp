#pragma once

#include <span>
#include <stdexcept>

#include <boost/endian/arithmetic.hpp>

#include "error.h"
#include "protocol.h"

namespace http2 {

#pragma pack(push, 1)
struct settings_item {
  settings_item() = delete;
  settings_item(const settings_item &) = default;
  settings_item(settings_item &&) = default;

  settings_item(settings_type t, uint32_t v) : type(t), value(boost::endian::native_to_big(v)) {}
  settings_type type;
  uint32_t value;
};

struct header {
  boost::endian::big_uint24_t length;
  frame_type type = frame_type::DATA;
  uint8_t flags = 0;
  uint32_t stream_id = 0;
  // frame data follow

  std::size_t payload_size() const { return length.value(); }

  void set_payload_size(uint32_t size) { length = size; }
};

struct data_frame : public header {
  std::span<const uint8_t> data() const;
};

struct header_frame : public header {
  std::span<const uint8_t> header_block() const;
};

struct priority_frame : public header {
  uint32_t priority = 0;
  uint8_t weight = 0;
};

struct reset_frame : public header {
  error_code code;
};

struct settings_frame : public header {
  std::span<const settings_item> items() const {
    auto count = payload_size() / sizeof(settings_item);
    return {reinterpret_cast<const settings_item *>(reinterpret_cast<const char *>(this) + sizeof(header)), count};
  }
};

struct pushpromise_frame : public header {
  // not impl yet
};

struct ping_frame : public header {
  uint8_t data[8];
};

struct goaway_frame : public header {
  uint32_t last_stream_id = 0;
  error_code error = error_code::IS_OK;

  std::span<const uint8_t> additional() const {
    auto sz = payload_size() + sizeof(header) - sizeof(goaway_frame);
    return {reinterpret_cast<const uint8_t *>(this) + sizeof(goaway_frame), sz};
  }
};

struct window_update_frame : public header {
  boost::endian::big_uint32_t window_size = 0;
};

struct continuation_frame : public header {
  std::span<const uint8_t> header_block() const;
};

#pragma pack(pop)

template <frame_type type_id> struct FrameType {
  using type = header;
};

template <> struct FrameType<frame_type::DATA> {
  using type = data_frame;
};

template <> struct FrameType<frame_type::HEADERS> {
  using type = header_frame;
};

template <> struct FrameType<frame_type::PRIORITY> {
  using type = priority_frame;
};

template <> struct FrameType<frame_type::RST_STREAM> {
  using type = reset_frame;
};

template <> struct FrameType<frame_type::SETTINGS> {
  using type = settings_frame;
};

template <> struct FrameType<frame_type::PUSH_PROMISE> {
  using type = pushpromise_frame;
};

template <> struct FrameType<frame_type::PING> {
  using type = ping_frame;
};

template <> struct FrameType<frame_type::GOAWAY> {
  using type = goaway_frame;
};

template <> struct FrameType<frame_type::WINDOW_UPDATE> {
  using type = window_update_frame;
};

template <> struct FrameType<frame_type::CONTINUATION> {
  using type = continuation_frame;
};

class frame_analyzer {
public:
  frame_analyzer() = delete;
  frame_analyzer(const frame_analyzer &) = delete;
  frame_analyzer(frame_analyzer &&) = default;
  ~frame_analyzer() = default;

  /**
   * @brief from_buffer creates a frame_adaptor from raw data.
   * @param buffer contains raw data. data size must be not less that
   * min_size()
   * @return
   */
  [[nodiscard]] static frame_analyzer from_buffer(std::span<const uint8_t> buffer);

  /**
   * @brief min_size
   * @return minimal bytes count that can be used for from_buffer call.
   */
  [[nodiscard]] constexpr std::size_t static min_size() { return sizeof(struct header); }

  /**
   * @brief is_complete
   * @return true if frame_adaptor contains complete frame
   */
  bool is_complete() const { return size() <= buffer.size_bytes(); }

  /**
   * @brief check throws an exception when a frame is invalid
   */
  void check(std::size_t max_frame_size) const;

  /**
   * @brief get_frame only for case when a frame is complete
   * @return const refernce on a frame corresponded struct
   */
  template <frame_type type> const typename FrameType<type>::type &get_frame() const {
    const header &h = *reinterpret_cast<const struct header *>(buffer.data());
    if (h.type != type) {
      throw std::runtime_error("Can't cast to frame type");
    }
    return reinterpret_cast<const typename FrameType<type>::type &>(h);
  }

  /**
   * @brief frame_header
   * @return const refernce on frame header struct.
   * This function is allowed even if a frame is not complete
   */
  const header &frame_header() const { return *reinterpret_cast<const struct header *>(buffer.data()); }

  /**
   * @brief size
   * @return complete frame size.
   * This function is allowed even if is_complete() is false
   */
  std::size_t size() const { return sizeof(struct header) + frame_header().payload_size(); }

  /**
   * @brief raw_bytes
   * @return a span that contains all bytes of the frame.
   * if the frame isn't complete returns present bytes only.
   */
  std::span<const uint8_t> raw_bytes() const { return buffer; }

  /**
   * @brief payload
   * @return frame payload as span.
   * Can be not complete or even an empty
   * when a frame_adaptor is not complete
   */
  std::span<const uint8_t> payload() const { return buffer.subspan(sizeof(struct header)); }

private:
  explicit frame_analyzer(std::span<const uint8_t> b);

  void check_data_frame() const;
  void check_headers_frame() const;
  void check_priority_frame() const;
  void check_reset_frame() const;
  void check_settings_frame() const;
  void check_push_frame() const;
  void check_ping_frame() const;
  void check_goaway_frame() const;
  void check_window_update_frame() const;
  void check_continuation_frame() const;

private:
  using check_ptr = decltype(&frame_analyzer::check_data_frame);
  static check_ptr check_table[];

  std::span<const uint8_t> buffer;
};

} // namespace http2
