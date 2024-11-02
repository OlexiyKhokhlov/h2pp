#include <boost/test/unit_test.hpp>

#include <hpack/string.h>

BOOST_AUTO_TEST_SUITE(HPack_String)

BOOST_AUTO_TEST_CASE(Decode_Invalid_Data) {
  const std::vector<uint8_t> empty;
  BOOST_REQUIRE_THROW(rfc7541::string::decode(empty), std::invalid_argument);

  const std::vector<uint8_t> truncated = {0x05, 'H', 'e', 'l'};
  BOOST_REQUIRE_THROW(rfc7541::string::decode(truncated), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Decode_Empty_String) {
  const std::vector<uint8_t> one_byte = {0x00};
  const auto decoded_str = rfc7541::string::decode(one_byte);
  BOOST_CHECK_EQUAL(decoded_str.used_bytes, one_byte.size());
  BOOST_CHECK(decoded_str.value.empty());
}

BOOST_AUTO_TEST_CASE(Decode_non_ASCII_inplaced) {
  const std::vector<uint8_t> datastr = {0x04, 0x0, 0x1, 0xfe, 0xff};

  const auto decoded_str = rfc7541::string::decode(datastr);
  const std::vector<uint8_t> expected(datastr.begin() + 1, datastr.end());

  BOOST_CHECK_EQUAL(decoded_str.used_bytes, datastr.size());
  BOOST_TEST(decoded_str.value == expected);
}

BOOST_AUTO_TEST_CASE(Decode_inplaced) {
  const std::vector<uint8_t> datastr = {0x05, 'H', 'e', 'l', 'l', 'o'};

  const auto decoded_str = rfc7541::string::decode(datastr);
  const std::vector<uint8_t> expected(datastr.begin() + 1, datastr.end());

  BOOST_CHECK_EQUAL(decoded_str.used_bytes, datastr.size());
  BOOST_TEST(decoded_str.value == expected);
}

BOOST_AUTO_TEST_CASE(Decode_Huffman_encoded) {
  // 6 zero bytes are a prefix since  decoder reads a several bytes before by performance reason
  constexpr auto empty_prefix_size = 6;
  const std::vector<uint8_t> datastr = {0,    0,    0,    0,    0,    0, // 6 bytes prefix
                                        0x8c, 0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a, 0x6b, 0xa0, 0xab, 0x90, 0xf4, 0xff};
  const auto expected_str = "www.example.com";
  const std::vector<uint8_t> expected(expected_str, expected_str + strlen(expected_str));

  auto encoded_data = std::span<const uint8_t>(datastr).subspan(empty_prefix_size);
  const auto decoded_str = rfc7541::string::decode(encoded_data);

  BOOST_CHECK_EQUAL(decoded_str.used_bytes, encoded_data.size());
  BOOST_TEST(decoded_str.value == expected);
}

BOOST_AUTO_TEST_SUITE_END()
