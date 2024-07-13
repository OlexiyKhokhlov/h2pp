#define BOOST_TEST_MODULE Utils
#include <boost/test/unit_test.hpp>

#include <algorithm>
#include <cstdint>
#include <numeric>

#include <utils/buffer.h>
#include <utils/endianess.h>
#include <utils/streambuf.h>
#include <utils/utils.h>

using namespace utils;

BOOST_AUTO_TEST_SUITE(Free_functions)

BOOST_AUTO_TEST_CASE(Make_masks_for_uint8) {
  using value_type = uint8_t;

  BOOST_CHECK_EQUAL(make_mask<value_type>(0), 0);
  BOOST_CHECK_EQUAL(make_mask<value_type>(1), 1);
  BOOST_CHECK_EQUAL(make_mask<value_type>(2), 0b11);
  BOOST_CHECK_EQUAL(make_mask<value_type>(sizeof(value_type) * 8 / 2), 0x0f);
  BOOST_CHECK_EQUAL(make_mask<value_type>(sizeof(value_type) * 8 - 1), 0x7f);
  BOOST_CHECK_EQUAL(make_mask<value_type>(sizeof(value_type) * 8), std::numeric_limits<value_type>::max());
}

BOOST_AUTO_TEST_CASE(Make_masks_for_uint32) {
  using value_type = uint32_t;

  BOOST_CHECK_EQUAL(make_mask<value_type>(0), 0);
  BOOST_CHECK_EQUAL(make_mask<value_type>(1), 1);
  BOOST_CHECK_EQUAL(make_mask<value_type>(2), 0b11);
  BOOST_CHECK_EQUAL(make_mask<value_type>(sizeof(value_type) * 8 / 2), 0x0ffff);
  BOOST_CHECK_EQUAL(make_mask<value_type>(sizeof(value_type) * 8 - 1), 0x7fffffff);
  BOOST_CHECK_EQUAL(make_mask<value_type>(sizeof(value_type) * 8), std::numeric_limits<value_type>::max());
}

BOOST_AUTO_TEST_CASE(Make_masks_for_uint64) {
  using value_type = uint64_t;

  BOOST_CHECK_EQUAL(make_mask<value_type>(0), 0);
  BOOST_CHECK_EQUAL(make_mask<value_type>(1), 1);
  BOOST_CHECK_EQUAL(make_mask<value_type>(2), 0b11);
  BOOST_CHECK_EQUAL(make_mask<value_type>(sizeof(value_type) * 8 / 2), 0x0ffffffff);
  BOOST_CHECK_EQUAL(make_mask<value_type>(sizeof(value_type) * 8 - 1), 0x7fffffffffffffff);
  BOOST_CHECK_EQUAL(make_mask<value_type>(sizeof(value_type) * 8), std::numeric_limits<value_type>::max());
}

BOOST_AUTO_TEST_CASE(Сeil_order2_0) {
  BOOST_CHECK_EQUAL(ceil_order2(0, 0), 0);
  BOOST_CHECK_EQUAL(ceil_order2(1, 0), 1);
  BOOST_CHECK_EQUAL(ceil_order2(2, 0), 2);
  BOOST_CHECK_EQUAL(ceil_order2(1000, 0), 1000);
}

BOOST_AUTO_TEST_CASE(Сeil_order2_1) {
  BOOST_CHECK_EQUAL(ceil_order2(0, 1), 0);
  BOOST_CHECK_EQUAL(ceil_order2(1, 1), 2);
  BOOST_CHECK_EQUAL(ceil_order2(2, 1), 2);
  BOOST_CHECK_EQUAL(ceil_order2(3, 1), 4);
  BOOST_CHECK_EQUAL(ceil_order2(16, 1), 16);
  BOOST_CHECK_EQUAL(ceil_order2(17, 1), 18);
  BOOST_CHECK_EQUAL(ceil_order2(1000, 1), 1000);
}

BOOST_AUTO_TEST_CASE(Сeil_order2_64) {
  BOOST_CHECK_EQUAL(ceil_order2(0, 6), 0);
  BOOST_CHECK_EQUAL(ceil_order2(1, 6), 64);
  BOOST_CHECK_EQUAL(ceil_order2(2, 6), 64);
  BOOST_CHECK_EQUAL(ceil_order2(64, 6), 64);
  BOOST_CHECK_EQUAL(ceil_order2(100, 6), 128);
  BOOST_CHECK_EQUAL(ceil_order2(128, 6), 128);
  BOOST_CHECK_EQUAL(ceil_order2(635, 6), 640);
}

BOOST_AUTO_TEST_CASE(Сeil_order2_4096) {
  BOOST_CHECK_EQUAL(ceil_order2(0, 12), 0);
  BOOST_CHECK_EQUAL(ceil_order2(1, 12), 4096);
  BOOST_CHECK_EQUAL(ceil_order2(2, 12), 4096);
  BOOST_CHECK_EQUAL(ceil_order2(64, 12), 4096);
  BOOST_CHECK_EQUAL(ceil_order2(4095, 12), 4096);
  BOOST_CHECK_EQUAL(ceil_order2(4096, 12), 4096);
  BOOST_CHECK_EQUAL(ceil_order2(40959, 12), 40960);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Buffer)

BOOST_AUTO_TEST_CASE(Buffer_Creation) {
  BOOST_CHECK_NO_THROW(buffer(0));
  BOOST_CHECK_NO_THROW(buffer(1));
  BOOST_CHECK_NO_THROW(buffer(2));
  BOOST_CHECK_NO_THROW(buffer(1000));
  BOOST_CHECK_NO_THROW(buffer(1000000));
}

BOOST_AUTO_TEST_CASE(Buffer_Move) {
  buffer buf(64);
  auto buf2(std::move(buf));

  BOOST_CHECK_EQUAL(buf.max_size(), 0);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 0);
  BOOST_CHECK_EQUAL(buf2.max_size(), 64);
  BOOST_CHECK_EQUAL(buf2.prepare().size(), 64);

  auto buf3 = std::move(buf2);
  BOOST_CHECK_EQUAL(buf2.max_size(), 0);
  BOOST_CHECK_EQUAL(buf2.prepare().size(), 0);
  BOOST_CHECK_EQUAL(buf3.max_size(), 64);
  BOOST_CHECK_EQUAL(buf3.prepare().size(), 64);
}

BOOST_AUTO_TEST_CASE(Buffer_Empty) {
  buffer buf(100);

  BOOST_CHECK_EQUAL(buf.max_size(), 100);

  BOOST_CHECK_EQUAL(buf.prepare().size(), 100);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 0);

  buf.consume(0);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 100);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 0);

  buf.consume(1);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 100);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 0);

  buf.consume(10);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 100);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 0);

  buf.consume(100);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 100);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 0);
}

BOOST_AUTO_TEST_CASE(Buffer_Full) {
  buffer buf(100);

  BOOST_CHECK_EQUAL(buf.max_size(), 100);

  BOOST_CHECK_EQUAL(buf.commit(100), 100);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 100);

  BOOST_CHECK_EQUAL(buf.commit(0), 0);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 100);

  BOOST_CHECK_EQUAL(buf.commit(1), 0);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 100);

  BOOST_CHECK_EQUAL(buf.commit(10), 0);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 100);
}

BOOST_AUTO_TEST_CASE(Buffer_Commit) {
  buffer buf(64);

  auto initial_data = buf.prepare();

  BOOST_CHECK_EQUAL(buf.prepare().size(), 64);

  BOOST_CHECK_EQUAL(buf.commit(0), 0);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 64);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 0);
  BOOST_CHECK_EQUAL(buf.data_view().data(), initial_data.data());

  BOOST_CHECK_EQUAL(buf.commit(1), 1);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 63);
  BOOST_CHECK_EQUAL(buf.prepare().data(), initial_data.data() + 1);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 1);
  BOOST_CHECK_EQUAL(buf.data_view().data(), initial_data.data());

  BOOST_CHECK_EQUAL(buf.commit(2), 2);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 61);
  BOOST_CHECK_EQUAL(buf.prepare().data(), initial_data.data() + 3);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 3);
  BOOST_CHECK_EQUAL(buf.data_view().data(), initial_data.data());

  BOOST_CHECK_EQUAL(buf.commit(29), 29);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 32);
  BOOST_CHECK_EQUAL(buf.prepare().data(), initial_data.data() + 32);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 32);
  BOOST_CHECK_EQUAL(buf.data_view().data(), initial_data.data());

  BOOST_CHECK_EQUAL(buf.commit(64), 32);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 0);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 64);
  BOOST_CHECK_EQUAL(buf.data_view().data(), initial_data.data());
}

BOOST_AUTO_TEST_CASE(Buffer_Commit_with_data) {
  buffer buf(64);
  std::vector<uint8_t> data(128);

  auto initial_data = buf.prepare();
  std::fill(initial_data.begin(), initial_data.end(), 0xff);
  std::iota(data.begin(), data.end(), 0);

  BOOST_CHECK_EQUAL(buf.prepare().size(), 64);

  BOOST_CHECK_EQUAL(buf.commit(std::span<const uint8_t>()), 0);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 64);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 0);
  BOOST_CHECK_EQUAL(buf.data_view().data(), initial_data.data());

  BOOST_CHECK_EQUAL(buf.commit(std::span<const uint8_t>(data.begin(), 1)), 1);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 63);
  BOOST_CHECK_EQUAL(buf.prepare().data(), initial_data.data() + 1);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 1);
  BOOST_CHECK_EQUAL(buf.data_view().data(), initial_data.data());
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.data_view().begin(), buf.data_view().end(), data.begin(), data.begin() + 1);

  BOOST_CHECK_EQUAL(buf.commit(std::span<const uint8_t>(data.begin() + 1, 2)), 2);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 61);
  BOOST_CHECK_EQUAL(buf.prepare().data(), initial_data.data() + 3);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 3);
  BOOST_CHECK_EQUAL(buf.data_view().data(), initial_data.data());
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.data_view().begin(), buf.data_view().end(), data.begin(), data.begin() + 3);

  BOOST_CHECK_EQUAL(buf.commit(std::span<const uint8_t>(data.begin() + 3, 29)), 29);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 32);
  BOOST_CHECK_EQUAL(buf.prepare().data(), initial_data.data() + 32);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 32);
  BOOST_CHECK_EQUAL(buf.data_view().data(), initial_data.data());
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.data_view().begin(), buf.data_view().end(), data.begin(), data.begin() + 32);

  BOOST_CHECK_EQUAL(buf.commit(std::span<const uint8_t>(data.begin() + 32, 64)), 32);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 0);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 64);
  BOOST_CHECK_EQUAL(buf.data_view().data(), initial_data.data());
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.data_view().begin(), buf.data_view().end(), data.begin(), data.begin() + 64);
}

BOOST_AUTO_TEST_CASE(Buffer_Consume) {
  buffer buf(64);
  auto initial_data = buf.prepare();
  std::iota(initial_data.begin(), initial_data.end(), 0);
  std::vector<uint8_t> data(64);
  std::iota(data.begin(), data.end(), 0);

  buf.commit(32);
  buf.consume(0);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 32);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 32);
  BOOST_CHECK_EQUAL(buf.data_view().data(), initial_data.data());
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.data_view().begin(), buf.data_view().end(), data.begin(), data.begin() + 32);

  buf.consume(128);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 64);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 0);
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.prepare().begin(), buf.prepare().end(), data.begin(), data.end());

  buf.commit(32);
  buf.consume(64);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 64);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 0);
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.prepare().begin(), buf.prepare().end(), data.begin(), data.end());

  buf.commit(64);
  buf.consume(64);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 64);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 0);
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.prepare().begin(), buf.prepare().end(), data.begin(), data.end());

  buf.consume(64);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 64);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 0);
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.prepare().begin(), buf.prepare().end(), data.begin(), data.end());

  buf.commit(64);
  buf.consume(32);
  BOOST_CHECK_EQUAL(buf.prepare().size(), 32);
  BOOST_CHECK_EQUAL(buf.data_view().size(), 32);
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.prepare().begin(), buf.prepare().end(), data.begin() + 32, data.end());
  BOOST_CHECK_EQUAL_COLLECTIONS(buf.data_view().begin(), buf.data_view().end(), data.begin() + 32, data.end());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(Streambuf)
BOOST_AUTO_TEST_CASE(Streambuf_Create) {
  BOOST_CHECK_NO_THROW(streambuf());
  streambuf buf;
  BOOST_CHECK_EQUAL(buf.flush().size(), 0);
}

BOOST_AUTO_TEST_CASE(Streambuf_Push_back) {
  streambuf buf;
  std::vector<uint8_t> data(4096 * 2);
  std::iota(data.begin(), data.end(), 0);

  buf.push_back(std::span<const uint8_t>());
  BOOST_CHECK_EQUAL(buf.flush().size(), 0);

  buf.push_back({data.begin(), data.begin() + 4});
  auto buffs = buf.flush();
  BOOST_CHECK_EQUAL(buffs.size(), 1);
  BOOST_CHECK_EQUAL(buffs.front().max_size(), 64);
  BOOST_CHECK_EQUAL(buffs.front().data_view().size(), 4);
  BOOST_CHECK_EQUAL_COLLECTIONS(buffs.front().data_view().begin(), buffs.front().data_view().end(), data.begin(),
                                data.begin() + 4);

  buf.push_back({data.begin(), data.begin() + 63});
  buf.push_back({data.begin() + 63, data.begin() + 63 + 2});
  buffs = buf.flush();
  BOOST_CHECK_EQUAL(buffs.size(), 2);
  BOOST_CHECK_EQUAL(buffs.front().max_size(), 64);
  BOOST_CHECK_EQUAL(buffs.front().data_view().size(), 64);
  BOOST_CHECK_EQUAL_COLLECTIONS(buffs.front().data_view().begin(), buffs.front().data_view().end(), data.begin(),
                                data.begin() + 64);
  BOOST_CHECK_EQUAL(buffs.back().max_size(), 128);
  BOOST_CHECK_EQUAL(buffs.back().data_view().size(), 1);

  buf.push_back({data.begin(), data.begin() + 126});
  buf.push_back({data.begin() + 126, data.begin() + 126 + 4});
  buffs = buf.flush();
  BOOST_CHECK_EQUAL(buffs.size(), 2);
  BOOST_CHECK_EQUAL(buffs.front().max_size(), 128);
  BOOST_CHECK_EQUAL(buffs.front().data_view().size(), 128);
  BOOST_CHECK_EQUAL_COLLECTIONS(buffs.front().data_view().begin(), buffs.front().data_view().end(), data.begin(),
                                data.begin() + 128);
  BOOST_CHECK_EQUAL(buffs.back().max_size(), 256);
  BOOST_CHECK_EQUAL(buffs.back().data_view().size(), 2);

  buf.push_back({data.begin(), data.end()});
  buffs = buf.flush();
  BOOST_CHECK_EQUAL(buffs.size(), 1);
  BOOST_CHECK_EQUAL_COLLECTIONS(buffs.front().data_view().begin(), buffs.front().data_view().end(), data.begin(),
                                data.end());

  buf.push_back({data.begin(), data.end()});
  buf.push_back({data.begin(), data.begin() + 1});
  buffs = buf.flush();
  BOOST_CHECK_EQUAL(buffs.size(), 2);
  BOOST_CHECK_EQUAL(buffs.back().max_size(), 4096);
  BOOST_CHECK_EQUAL(buffs.back().data_view().size(), 1);
}
BOOST_AUTO_TEST_SUITE_END()
