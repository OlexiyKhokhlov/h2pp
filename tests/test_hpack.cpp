#define BOOST_TEST_MODULE HPack
#include <boost/test/unit_test.hpp>

#include <algorithm>

#include <hpack/bitstream.h>
#include <hpack/constants.h>
#include <hpack/decoder.h>
#include <hpack/encoder.h>
#include <hpack/huffman.h>
#include <hpack/integer.h>
#include <hpack/string.h>

BOOST_AUTO_TEST_SUITE(Huffman_Codes)

BOOST_AUTO_TEST_CASE(Encode_Decode_All_symbols) {
  // Encode & decode all allowed symbols
  for (uint16_t value = 0; value < 256; ++value) {
    const auto huff_code = rfc7541::huffman::encode(value);
    const auto decoded_value = rfc7541::huffman::decode(huff_code);
    BOOST_CHECK(decoded_value.has_value());
    BOOST_CHECK_EQUAL(value, decoded_value.value());
  }
}

BOOST_AUTO_TEST_CASE(Decode_wrong_codes) {
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

BOOST_AUTO_TEST_SUITE(Decode_integer)
/*
BOOST_AUTO_TEST_CASE(read_command) {
  {
    std::vector<uint8_t> index_data = {0x80};
    rfc7541::ibitstream in(index_data);
    rfc7541::command cmd;
    in >> cmd;
    BOOST_CHECK_EQUAL((unsigned)cmd, (unsigned)rfc7541::command::INDEX);
  }

  {
    std::vector<uint8_t> literal_incremental_index_data = {0x4F};
    rfc7541::ibitstream in(literal_incremental_index_data);
    rfc7541::command cmd;
    in >> cmd;
    BOOST_CHECK_EQUAL((unsigned)cmd, (unsigned)rfc7541::command::LITERAL_INCREMENTAL_INDEX);
  }

  {
    std::vector<uint8_t> change_table_size_data = {0x30};
    rfc7541::ibitstream in(change_table_size_data);
    rfc7541::command cmd;
    in >> cmd;
    BOOST_CHECK_EQUAL((unsigned)cmd, (unsigned)rfc7541::command::CHANGE_TABLE_SIZE);
  }

  {
    std::vector<uint8_t> literal_without_index_data = {0x0F};
    rfc7541::ibitstream in(literal_without_index_data);
    rfc7541::command cmd;
    in >> cmd;
    BOOST_CHECK_EQUAL((unsigned)cmd, (unsigned)rfc7541::command::LITERAL_WITHOUT_INDEX);
  }

  {
    std::vector<uint8_t> literal_never_index_data = {0x1C};
    rfc7541::ibitstream in(literal_never_index_data);
    rfc7541::command cmd;
    in >> cmd;
    BOOST_CHECK_EQUAL((unsigned)cmd, (unsigned)rfc7541::command::LITERAL_NEVER_INDEX);
  }
}
*/
BOOST_AUTO_TEST_CASE(read_integer) {
  {
    std::vector<uint8_t> datanr = {0x2A};

    auto result = rfc7541::integer::decode(1, datanr);

    BOOST_CHECK_EQUAL(result.used_bytes, 1);
    BOOST_CHECK_EQUAL(result.value, 42);
  }

  {
    std::vector<uint8_t> datanr = {0x2A};

    auto result = rfc7541::integer::decode(3, datanr);

    BOOST_CHECK_EQUAL(result.used_bytes, 1);
    BOOST_CHECK_EQUAL(result.value, 10);
  }

  {
    std::vector<uint8_t> datanr = {0x3f, 0x9a, 0x0a};

    auto result = rfc7541::integer::decode(3, datanr);

    BOOST_CHECK_EQUAL(result.used_bytes, 3);
    BOOST_CHECK_EQUAL(result.value, 1337);
  }
}

BOOST_AUTO_TEST_CASE(read_string) {
  {
    std::vector<uint8_t> datastr = {0x05, 'H', 'e', 'l', 'l', 'o'};

    auto decoded_str = rfc7541::string::decode(datastr);
    std::vector<uint8_t> expected(datastr.begin() + 1, datastr.end());

    BOOST_CHECK_EQUAL(decoded_str.used_bytes, datastr.size());
    BOOST_TEST(decoded_str.value == expected);
  }

  {
    std::vector<uint8_t> datastr = {0x8c, 0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a, 0x6b, 0xa0, 0xab, 0x90, 0xf4, 0xff};

    auto decoded_str = rfc7541::string::decode(datastr);

    const auto expected_str = "www.example.com";
    std::vector<uint8_t> expected(expected_str, expected_str + strlen(expected_str));

    BOOST_CHECK_EQUAL(decoded_str.used_bytes, datastr.size());
    BOOST_TEST(decoded_str.value == expected);
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Decoder_Encoder)

struct TestData {
  std::deque<rfc7541::header_field> fields;
  std::vector<uint8_t> encoded_data;
};

std::vector<TestData> encoded_with_huffman_data = {
    {// Request 1
     {{":method", "GET"}, {":scheme", "http"}, {":path", "/"}, {":authority", "www.example.com"}},
     {0x82,                                     // command::INDEX {":method", "GET"}
      0x86,                                     // command::INDEX {":scheme", "http"},
      0x84,                                     // command::INDEX {":path", "/"}
      0x41,                                     // command::LITERAL_INCREMENTAL_INDEX i=1 ":authority"
                                                //
      0x8c,                                     // huffman encoded len = 12
                                                // "www.example.com"
      0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a, 0x6b, //
      0xa0, 0xab, 0x90, 0xf4, 0xff}},
    {// Request 2
     {{":method", "GET"},
      {":scheme", "http"},
      {":path", "/"},
      {":authority", "www.example.com"},
      {"cache-control", "no-cache"}},
     {0x82, 0x86, 0x84, 0xbe, 0x58, 0x86, 0xa8, 0xeb, 0x10, 0x64, 0x9c, 0xbf}},
    {// Request 3
     {{":method", "GET"},
      {":scheme", "https"},
      {":path", "/index.html"},
      {":authority", "www.example.com"},
      {"custom-key", "custom-value"}},
     {0x82, 0x87, 0x85, 0xbf, 0x40, 0x88, 0x25, 0xa8, 0x49, 0xe9, 0x5b, 0xa9,
      0x7d, 0x7f, 0x89, 0x25, 0xa8, 0x49, 0xe9, 0x5b, 0xb8, 0xe8, 0xb4, 0xbf}},
};

// auto operator!=(const header_field &lhs, const header_field &rhs) {
//   return !(std::tie(lhs.name().size(), lhs.value()) ==
//            std::tie(rhs.name().size(), rhs.value()));
// }

BOOST_AUTO_TEST_CASE(EncodeDecode_with_huffman) {
  rfc7541::decoder decoder;
  rfc7541::encoder encoder;

  for (const auto &test_request : encoded_with_huffman_data) {
    // Decoding
    auto decoded_headers = decoder.decode(test_request.encoded_data);
    BOOST_CHECK_EQUAL(decoded_headers.size(), test_request.fields.size());
    //   BOOST_CHECK_EQUAL_COLLECTIONS(
    //       decoded_headers.begin(), decoded_headers.end(),
    //       test_request.fields.begin(), test_request.fields.end());

    auto [encoded_buf, encoded_fields_count] = encoder.encode(test_request.fields, 4096);
    BOOST_CHECK_EQUAL(encoded_fields_count, test_request.fields.size());
    BOOST_CHECK(!encoded_buf.empty());
    auto data = encoded_buf.front().data_view();
    BOOST_CHECK_EQUAL(data.size(), test_request.encoded_data.size());

    BOOST_CHECK_EQUAL_COLLECTIONS(data.begin(), data.end(), test_request.encoded_data.begin(),
                                  test_request.encoded_data.end());
  }
}

BOOST_AUTO_TEST_CASE(TestDecoder) {
  std::vector<uint8_t> encoded_data = {
      0x82, 0x87, 0x84, 0x41, 0x8b, 0xf1, 0xe3, 0xc2, 0xf3, 0x19, 0x33, 0xdb, 0x1a, 0xe4, 0x3d, 0x3f,
  };
  // std::vector<uint8_t> encoded_data = {0x42, 0x82, 0x98, 0xa9};
  rfc7541::decoder decoder;
  auto decoded_headers = decoder.decode(encoded_data);
  for (const auto &h : decoded_headers) {
    std::cout << h.name_view() << ": " << h.value_view();
  }
}

BOOST_AUTO_TEST_CASE(Decode_Sample) {
  std::vector<uint8_t> data = {0x87, 0x41, 0x8b, 0xf1, 0xe3, 0xc2, 0xf3, 0x1c, 0xf3, 0x50,
                               0x55, 0xc8, 0x7a, 0x7f, 0x84, 0x82, 0x53, 0x03, 0x2a, 0x2f,
                               0x2a, 0x7a, 0x87, 0x9c, 0x55, 0xd6, 0xc0, 0x17, 0x02, 0xe1};

  rfc7541::decoder decoder;
  auto header = decoder.decode(data);
  std::cout << std::endl;
  std::cout << std::endl;
  std::cout << std::endl;
  for (const auto &h : header) {
    std::cout << "\t" << h.name_view() << ": " << h.value_view() << std::endl;
  }
}

// BOOST_AUTO_TEST_CASE(TestEncDec) {
//   std::deque<rfc7541::header_field> fields = {
//       {"user-agent", "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) "
//                      "Gecko/20100101 Firefox/118.0"}};

//   rfc7541::encoder encoder;
//   rfc7541::decoder decoder;

//   auto [buffer, count] = encoder.encode(fields, 4096);

//   auto header = decoder.decode(buffer.front().data_view());
// }
// BOOST_AUTO_TEST_CASE(Decoding_without_huffman) {
//   // C.3
//   // All request without huffman coding
//   std::vector<uint8_t> request1 = {0x82, 0x86, 0x84, 0x41, 0x0f, 0x77, 0x77,
//                                    0x77, 0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70,
//                                    0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d};

//   rfc7541::decoder decoder;
//   auto header = decoder.decode(request1);
//   std::cout << "Request1:" << std::endl;
//   for (const auto &h : header) {
//     std::cout << h.name_view() << " " << h.value_view() << std::endl;
//   }

//   std::vector<uint8_t> request2 = {0x82, 0x86, 0x84, 0xbe, 0x58, 0x08, 0x6e,
//                                    0x6f, 0x2d, 0x63, 0x61, 0x63, 0x68, 0x65};
//   header = decoder.decode(request2);
//   std::cout << "Request2:" << std::endl;
//   for (const auto &h : header) {
//     std::cout << h.name_view() << " " << h.value_view() << std::endl;
//   }

//   std::vector<uint8_t> request3 = {
//       0x82, 0x87, 0x85, 0xbf, 0x40, 0x0a, 0x63, 0x75, 0x73, 0x74,
//       0x6f, 0x6d, 0x2d, 0x6b, 0x65, 0x79, 0x0c, 0x63, 0x75, 0x73,
//       0x74, 0x6f, 0x6d, 0x2d, 0x76, 0x61, 0x6c, 0x75, 0x65};
//   header = decoder.decode(request3);
//   std::cout << "Request3:" << std::endl;
//   for (const auto &h : header) {
//     std::cout << h.name_view() << " " << h.value_view() << std::endl;
//   }
// }

// Response: C. 6.1
//    std::vector<uint8_t> request1 = {0x48, 0x82, 0x64, 0x02, 0x58, 0x85, 0xae, 0xc3, 0x77,
//                                     0x1a, 0x4b, 0x61, 0x96, 0xd0, 0x7a, 0xbe, 0x94, 0x10,
//                                     0x54, 0xd4, 0x44, 0xa8, 0x20, 0x05, 0x95, 0x04, 0x0b,
//                                     0x81, 0x66, 0xe0, 0x82, 0xa6, 0x2d, 0x1b, 0xff, 0x6e,
//                                     0x91, 0x9d, 0x29, 0xad, 0x17, 0x18, 0x63, 0xc7, 0x8f,
//                                     0x0b, 0x97, 0xc8, 0xe9, 0xae, 0x82, 0xae, 0x43, 0xd3};

BOOST_AUTO_TEST_SUITE_END()
