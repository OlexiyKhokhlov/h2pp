#include <boost/test/unit_test.hpp>

#include <hpack/integer.h>

BOOST_AUTO_TEST_SUITE(HPack_Integer)

BOOST_AUTO_TEST_CASE(Decode_wrong_empty) {
  const std::vector<uint8_t> empty = {};
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(1, empty), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(2, empty), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(3, empty), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(4, empty), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(Decode_wrong_suffix) {
  const std::vector<uint8_t> raw = {0x01};
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(0, raw), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(5, raw), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(8, raw), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(16, raw), std::invalid_argument);
  BOOST_CHECK_EQUAL(1, rfc7541::integer::decode(1, raw).value);
}

BOOST_AUTO_TEST_CASE(Decode_wrong_length) {
  const std::vector<uint8_t> raw_one_byte = {0xFF};
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(1, raw_one_byte), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(2, raw_one_byte), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(3, raw_one_byte), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(4, raw_one_byte), std::invalid_argument);

  const std::vector<uint8_t> raw_few_bytes = {0xFF, 0x81};
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(1, raw_few_bytes), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(2, raw_few_bytes), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(3, raw_few_bytes), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(4, raw_few_bytes), std::invalid_argument);

  const std::vector<uint8_t> raw_3_bytes = {0xFF, 0xFF, 0x81};
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(1, raw_3_bytes), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(2, raw_3_bytes), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(3, raw_3_bytes), std::invalid_argument);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(4, raw_3_bytes), std::invalid_argument);

  const std::vector<uint8_t> raw_extra_long = {0x7f, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01};
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(1, raw_extra_long), std::overflow_error);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(2, raw_extra_long), std::overflow_error);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(3, raw_extra_long), std::overflow_error);
  BOOST_REQUIRE_THROW(rfc7541::integer::decode(4, raw_extra_long), std::overflow_error);
}

BOOST_AUTO_TEST_CASE(Decode_min_max) {
  const std::vector<uint8_t> raw_min = {0x00};
  BOOST_CHECK_EQUAL(0, rfc7541::integer::decode(1, raw_min).value);

  const std::vector<uint8_t> raw_max = {0x0f, 0xe1, 0xff, 0xff, 0x07};
  BOOST_CHECK_EQUAL(rfc7541::integer::MAX_HPACK_INT, rfc7541::integer::decode(4, raw_max).value);
}

BOOST_AUTO_TEST_CASE(Decode_extra_long) {
  {
    const std::vector<uint8_t> raw_extra_long = {0x7f, 0x80, 0x80, 0x80, 0x80, 0x00};
    const auto decoded = rfc7541::integer::decode(1, raw_extra_long);
    BOOST_CHECK_EQUAL(raw_extra_long.size(), decoded.used_bytes);
    BOOST_CHECK_EQUAL(0x7f, decoded.value);
  }
  {
    const std::vector<uint8_t> raw_extra_long = {0x7f, 0x80, 0x80, 0x80, 0x01};
    const auto decoded = rfc7541::integer::decode(4, raw_extra_long);
    BOOST_CHECK_EQUAL(raw_extra_long.size(), decoded.used_bytes);
    BOOST_CHECK_EQUAL(0xf + (uint64_t(1) << (3 * 7)), decoded.value);
  }
}

BOOST_AUTO_TEST_CASE(Decode_valid_numbers) {
  {
    const std::vector<uint8_t> raw = {0x2A};

    const auto result = rfc7541::integer::decode(1, raw);

    BOOST_CHECK_EQUAL(result.used_bytes, 1);
    BOOST_CHECK_EQUAL(result.value, 42);
  }

  {
    const std::vector<uint8_t> raw = {0x2A};

    const auto result = rfc7541::integer::decode(3, raw);

    BOOST_CHECK_EQUAL(result.used_bytes, 1);
    BOOST_CHECK_EQUAL(result.value, 10);
  }

  {
    const std::vector<uint8_t> raw = {0x3f, 0x9a, 0x0a};

    const auto result = rfc7541::integer::decode(3, raw);

    BOOST_CHECK_EQUAL(result.used_bytes, 3);
    BOOST_CHECK_EQUAL(result.value, 1337);
  }
}

BOOST_AUTO_TEST_CASE(Encode_max) {
  const auto result = rfc7541::integer::encode(0, 4, rfc7541::integer::MAX_HPACK_INT);
  BOOST_CHECK_EQUAL(result.length, 5);
}

BOOST_AUTO_TEST_SUITE_END()
