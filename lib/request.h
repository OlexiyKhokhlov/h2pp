#pragma once

#include <chrono>
#include <deque>
#include <initializer_list>
#include <string>
#include <variant>
#include <vector>

#include <boost/url.hpp>

#include "hpack/header_field.h"
#include "method.h"

namespace http2 {

class stream;
using namespace std::chrono_literals;

/**
 * @brief The request class represents a HTTP2 request.
 * An every request required to have a method, scheme, authority and path.
 * All of them are HTTP2 pseudo-headers.
 * method, scheme, authority - are parts of an url.
 * So a request must be constructed with the help of url and method.
 * When a given url doesn't have above named parts an exception will be trown.
 *
 * All other header fields should be added via
 * 'request &header(rfc7541::header_field &&)'
 * 'request &headers(std::initializer_list<rfc7541::header_field>)'
 * 'template <typename II> request &headers(II begin, II end)' methods.
 * @note All these methods add header fields with no checks. So be carefull since
 * can be added extra pseudo-headers, duplicated fields etc.
 *
 * A request body can be set as a sequence of slices. Where every slice can be 'std::vector<uint8_t>'
 * or 'std::string'. All slices independ of them types are stored in the order of additional.
 */
class request {
public:
  /**
   * @brief request creates a request by the given url and method.
   * @param view - is an boost::url_view that must have 'scheme', 'authority' and 'path'
   * Whan one from them is not present an exception invalid_argument will thrown.
   * @param m is a http2::method
   */
  request(boost::url_view view, http2::method m = http2::method::GET);

  /**
   * @brief request creates a request by the given url and method.
   * @param view - is an boost::url that must have 'scheme', 'authority' and 'path'
   * Whan one from them is not present an exception invalid_argument will thrown.
   * @param m is a http2::method
   */
  request(const boost::url &url, http2::method m = http2::method::GET);

  request(const request &) = delete;
  request &operator=(const request &) = delete;
  request(request &&);
  request &operator=(request &&);
  ~request();

  /**
   * @brief header moves a given header field into internal fields list
   * Has no checks so pseudo-header or duplicated header fields can be added with no error
   * @param hf
   * @return returns a refernce on the request instance so creation can be organised as a chain
   */
  request &header(rfc7541::header_field &&hf);

  /**
   * @brief headers copies a given header fields into internal fields list
   * Has no checks so pseudo-header or duplicated header fields can be added with no error
   * @param list is a list fields those should be added
   * @return returns a refernce on the request instance so creation can be organised as a chain
   */
  request &headers(std::initializer_list<rfc7541::header_field> list);

  /**
   * @brief headers moves a sequence of heder fileds by pair of iterators 'begin' and 'end' into internal fields list
   * Has no checks so pseudo-header or duplicated header fiels can be added with no error
   * @param begin
   * @param end
   * @return returns a refernce on the request instance so creation can be organised as a chain
   */
  template <typename II> request &headers(II begin, II end) {
    std::move(begin, end, std::back_inserter(header_list));
    return *this;
  }

  /**
   * @brief body moves a given vector of bytes into internal body list
   * @param v
   * @return returns a refernce on the request instance so creation can be organised as a chain
   */
  request &body(std::vector<uint8_t> &&v);

  /**
   * @brief body moves a given string into internal body list
   * @param v
   * @return returns a refernce on the request instance so creation can be organised as a chain
   */
  request &body(std::string &&s);

  /**
   * @brief bodies moves a sequence of std::string or std::vector<uint8_t> that defined by pair of iterator into
   * internal body list
   * @param begin
   * @param end
   * @return returns a refernce on the request instance so creation can be organised as a chain
   */
  template <typename II> request &bodies(II begin, II end) {
    while (begin != end) {
      body(std::move(*begin++));
    }
    return *this;
  }

  /**
   * @brief set_timeout set request timeout
   * I. e. a max time which client wait for a response when a request has been sent.
   * The default value is - 30s
   * @param ms
   */
  void set_timeout(std::chrono::milliseconds ms) noexcept { timeout_value = ms; }

  /**
   * @brief raw_headers
   * @return  returns a const ref std::deque of all stored  header fields
   */
  [[nodiscard]] const std::deque<rfc7541::header_field> &raw_headers() const noexcept { return header_list; };

  /**
   * @brief raw_body
   * @return returns a const ref deque of spans for all stored body slices
   */
  [[nodiscard]] const std::deque<std::span<const uint8_t>> &raw_body() const noexcept { return span_list; }

  /**
   * @brief body_size
   * @return returns a stored body size
   */
  [[nodiscard]] std::size_t body_size() const noexcept { return size; }

  /**
   * @brief timeout
   * @return returns a timeout value
   */
  [[nodiscard]] auto timeout() const noexcept { return timeout_value; }

private:
  // calling from stream
  void commit_headers(std::size_t n);
  void commit_body(std::size_t n);

private:
  std::deque<rfc7541::header_field> header_list;

  using block_type = std::variant</*std::monostate, */ std::string, std::vector<uint8_t>>;
  std::deque<block_type> body_list;
  std::deque<std::span<const uint8_t>> span_list;
  std::size_t size = 0;

  std::chrono::milliseconds timeout_value = 30s;

  friend stream;
};

} // namespace http2
