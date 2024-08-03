#include <vector>

#include <boost/test/unit_test.hpp>

#include <hpack/huffman.h>

BOOST_AUTO_TEST_SUITE(HPack_Huffman)

BOOST_AUTO_TEST_CASE(Encode_Decode_All_Symbols) {
  const auto allowed_length = rfc7541::huffman::allowed_code_lengths();

  for (uint16_t value = 0; value < 256; ++value) {
    const auto huff_code = rfc7541::huffman::encode(value);
    auto len_it = std::find(allowed_length.begin(), allowed_length.end(), huff_code.bitLength);
    BOOST_CHECK_MESSAGE(len_it != allowed_length.end(), "For symbol: " + std::to_string(value));

    const auto decoded_value = rfc7541::huffman::decode(huff_code);
    BOOST_CHECK_MESSAGE(decoded_value.has_value(), "For symbol: " + std::to_string(value));
    BOOST_CHECK_EQUAL(value, decoded_value.value());
  }
}

BOOST_AUTO_TEST_CASE(Decode_Wrong_Codes) {
  const auto allowed_length = rfc7541::huffman::allowed_code_lengths();

  // Decode wrong huffman codes
  for (uint8_t len = 0; len <= 32; ++len) {
    if (std::find(std::begin(allowed_length), std::end(allowed_length), len) == std::end(allowed_length)) {
      // code is ok, length is wrong
      BOOST_CHECK(!rfc7541::huffman::decode({0, len}).has_value());
    } else {
      // code is wrong, length is ok
      BOOST_CHECK(!rfc7541::huffman::decode({0xFF, len}).has_value());
    }
  }
}

BOOST_AUTO_TEST_SUITE_END()
