#define BOOST_TEST_MODULE HPack
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <tuple>

#include <hpack/bitstream.h>
#include <hpack/constants.h>
#include <hpack/decoder.h>
#include <hpack/encoder.h>

namespace rfc7541 {
bool operator==(const std::span<const uint8_t> &lhs, const std::span<const uint8_t> &rhs) noexcept {
  return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

bool operator!=(const std::span<const uint8_t> &lhs, const std::span<const uint8_t> &rhs) noexcept {
  return !(lhs == rhs);
}

bool operator==(const header_field &lhs, const header_field &rhs) noexcept {
  auto lname = lhs.name();
  auto lvalue = lhs.value();
  auto rname = rhs.name();
  auto rvalue = rhs.value();
  return lname == rname && lvalue == rvalue;
}

bool operator!=(const header_field &lhs, const header_field &rhs) noexcept { return !(lhs == rhs); }

std::ostream &operator<<(std::ostream &os, const header_field &hf) {
  os << hf.name_view();
  return os;
}

} // namespace rfc7541

BOOST_AUTO_TEST_SUITE(Decoder_Encoder)

struct TestData {
  std::deque<rfc7541::header_field> fields;
  std::vector<uint8_t> encoded_data;
};

constinit auto padding_size = 6;
std::vector<TestData> encoded_with_huffman_data = {
    // 6 zero bytes are a prefix since  decoder reads a several bytes before by performance reason
    // clang-format off
    {
      // Request 1
      {
         {":method", "GET"},
         {":scheme", "http"},
         {":path", "/"},
         {":authority", "www.example.com"}
      },
      {
         0, 0, 0, 0, 0, 0, // 6bytes padding padding
         0x82,             // command::INDEX {":method", "GET"}
         0x86,             // command::INDEX {":scheme", "http"},
         0x84,             // command::INDEX {":path", "/"}
         0x41,             // command::LITERAL_INCREMENTAL_INDEX i=1 ":authority"
         0x8c,             // huffman encoded len = 12
         0xf1, 0xe3, 0xc2, // Encoded string "www.example.com"
         0xe5, 0xf2, 0x3a,
         0x6b, 0xa0, 0xab,
         0x90, 0xf4, 0xff
      }
    },
    {
      // Request 2
      {
        {":method", "GET"},
        {":scheme", "http"},
        {":path", "/"},
        {":authority", "www.example.com"},
        {"cache-control", "no-cache"}
      },
      {
        0, 0, 0, 0, 0, 0, // 6bytes padding padding
        0x82, // command::INDEX {":method", "GET"}
        0x86, // command::INDEX {":scheme", "http"},
        0x84, // command::INDEX {":path", "/"}
        0xbe, 0x58, 0x86, 0xa8, 0xeb, 0x10, 0x64, 0x9c, 0xbf}
      },
    {
      // Request 3
      {
        {":method", "GET"},
        {":scheme", "https"},
        {":path", "/index.html"},
        {":authority", "www.example.com"},
        {"custom-key", "custom-value"}
      },
      {
        0, 0, 0, 0, 0, 0, // 6bytes padding padding
        0x82, // command::INDEX {":method", "GET"}
        0x87, // command::INDEX {":scheme", "https"},
        0x85, // command::INDEX {":path", "/index.html"}
        0xbf, 0x40, 0x88, 0x25, 0xa8, 0x49,
        0xe9, 0x5b, 0xa9, 0x7d, 0x7f, 0x89, 0x25, 0xa8, 0x49, 0xe9, 0x5b, 0xb8, 0xe8, 0xb4, 0xbf
      }
    },
};
// clang-format on

BOOST_AUTO_TEST_CASE(EncodeDecode_with_huffman) {
  rfc7541::decoder decoder;
  rfc7541::encoder encoder;

  for (const auto &test_request : encoded_with_huffman_data) {
    // Decoding
    const auto encoded_data = std::span<const uint8_t>(test_request.encoded_data).subspan(padding_size);
    const auto decoded_headers = decoder.decode(encoded_data);

    BOOST_CHECK_EQUAL(decoded_headers.size(), test_request.fields.size());
    // BOOST_CHECK(std::equal(decoded_headers.cbegin(), decoded_headers.cend(), test_request.fields.cbegin(),
    //                        test_request.fields.cend()));
    BOOST_CHECK_EQUAL_COLLECTIONS(decoded_headers.begin(), decoded_headers.end(), test_request.fields.begin(),
                                  test_request.fields.end());

    // Encoding
    const auto [encoded_buf, encoded_fields_count] = encoder.encode(test_request.fields, 4096);
    BOOST_CHECK_EQUAL(encoded_fields_count, test_request.fields.size());
    BOOST_CHECK(!encoded_buf.empty());
    const auto encoded_span = encoded_buf.front().data_view();
    BOOST_CHECK_EQUAL(encoded_span.size(), encoded_data.size());
    BOOST_CHECK_EQUAL_COLLECTIONS(encoded_span.begin(), encoded_span.end(), encoded_data.begin(), encoded_data.end());
  }
}

// BOOST_AUTO_TEST_CASE(TestDecoder) {
//   std::vector<uint8_t> encoded_data = {
//       0x82, 0x87, 0x84, 0x41, 0x8b, 0xf1, 0xe3, 0xc2, 0xf3, 0x19, 0x33, 0xdb, 0x1a, 0xe4, 0x3d, 0x3f,
//   };
//   // std::vector<uint8_t> encoded_data = {0x42, 0x82, 0x98, 0xa9};
//   rfc7541::decoder decoder;
//   auto decoded_headers = decoder.decode(encoded_data);
//   for (const auto &h : decoded_headers) {
//     std::cout << h.name_view() << ": " << h.value_view();
//   }
// }

// BOOST_AUTO_TEST_CASE(Decode_Sample) {
//   std::vector<uint8_t> data = {0x87, 0x41, 0x8b, 0xf1, 0xe3, 0xc2, 0xf3, 0x1c, 0xf3, 0x50,
//                                0x55, 0xc8, 0x7a, 0x7f, 0x84, 0x82, 0x53, 0x03, 0x2a, 0x2f,
//                                0x2a, 0x7a, 0x87, 0x9c, 0x55, 0xd6, 0xc0, 0x17, 0x02, 0xe1};

//   rfc7541::decoder decoder;
//   auto header = decoder.decode(data);
//   std::cout << std::endl;
//   std::cout << std::endl;
//   std::cout << std::endl;
//   for (const auto &h : header) {
//     std::cout << "\t" << h.name_view() << ": " << h.value_view() << std::endl;
//   }
// }

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
