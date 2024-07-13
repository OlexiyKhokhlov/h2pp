#include "static_table.h"

#include <algorithm>
#include <cstring>

namespace {

bool operator==(std::span<const char> lhs, std::span<const uint8_t> rhs) {
  return 0 == memcmp(lhs.data(), rhs.data(), rhs.size_bytes());
}

bool operator!=(std::span<const char> lhs, std::span<const uint8_t> rhs) {
  return 0 != memcmp(lhs.data(), rhs.data(), rhs.size_bytes());
}

consteval std::span<const char> operator""_sp(const char *str, std::size_t len) { return {str, len}; }

struct static_field {
  std::span<const char> name;
  std::span<const char> value;
};

static const static_field field_table[] = {
    {":authority"_sp, ""_sp},
    {":method"_sp, "GET"_sp},
    {":method"_sp, "POST"_sp},
    {":path"_sp, "/"_sp},
    {":path"_sp, "/index.html"_sp},
    {":scheme"_sp, "http"_sp},
    {":scheme"_sp, "https"_sp},
    {":status"_sp, "200"_sp},
    {":status"_sp, "204"_sp},
    {":status"_sp, "206"_sp},
    {":status"_sp, "304"_sp},
    {":status"_sp, "400"_sp},
    {":status"_sp, "404"_sp},
    {":status"_sp, "500"_sp},
    {"accept-charset"_sp, ""_sp},
    {"accept-encoding"_sp, "gzip, deflate"_sp},
    {"accept-language"_sp, ""_sp},
    {"accept-ranges"_sp, ""_sp},
    {"accept"_sp, ""_sp},
    {"access-control-allow-origin"_sp, ""_sp},
    {"age"_sp, ""_sp},
    {"allow"_sp, ""_sp},
    {"authorization"_sp, ""_sp},
    {"cache-control"_sp, ""_sp},
    {"content-disposition"_sp, ""_sp},
    {"content-encoding"_sp, ""_sp},
    {"content-language"_sp, ""_sp},
    {"content-length"_sp, ""_sp},
    {"content-location"_sp, ""_sp},
    {"content-range"_sp, ""_sp},
    {"content-type"_sp, ""_sp},
    {"cookie"_sp, ""_sp},
    {"date"_sp, ""_sp},
    {"etag"_sp, ""_sp},
    {"expect"_sp, ""_sp},
    {"expires"_sp, ""_sp},
    {"from"_sp, ""_sp},
    {"host"_sp, ""_sp},
    {"if-match"_sp, ""_sp},
    {"if-modified-since"_sp, ""_sp},
    {"if-none-match"_sp, ""_sp},
    {"if-range"_sp, ""_sp},
    {"if-unmodified-since"_sp, ""_sp},
    {"last-modified"_sp, ""_sp},
    {"link"_sp, ""_sp},
    {"location"_sp, ""_sp},
    {"max-forwards"_sp, ""_sp},
    {"proxy-authenticate"_sp, ""_sp},
    {"proxy-authorization"_sp, ""_sp},
    {"range"_sp, ""_sp},
    {"referer"_sp, ""_sp},
    {"refresh"_sp, ""_sp},
    {"retry-after"_sp, ""_sp},
    {"server"_sp, ""_sp},
    {"set-cookie"_sp, ""_sp},
    {"strict-transport-security"_sp, ""_sp},
    {"transfer-encoding"_sp, ""_sp},
    {"user-agent"_sp, ""_sp},
    {"vary"_sp, ""_sp},
    {"via"_sp, ""_sp},
    {"www-authenticate"_sp, ""_sp},
};

static const unsigned char asso_values[] = {
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 10, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    15, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 1,  66, 8,  13, 2,  41, 15, 5,  19, 66, 23, 2,  4,  12, 32, 20, 66, 3,  21,
    1,  23, 17, 32, 20, 14, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66,
    66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66, 66};

static const unsigned char lengthtable[] = {0,  0,  0,  3, 6,  6,  15, 4, 7,  14, 11, 7, 12, 16, 13, 8, 14,
                                            4,  3,  13, 5, 5,  7,  16, 8, 13, 16, 13, 8, 7,  15, 10, 4, 6,
                                            19, 13, 12, 7, 17, 10, 13, 6, 7,  18, 17, 4, 16, 27, 4,  4, 0,
                                            0,  10, 19, 0, 0,  0,  25, 0, 0,  0,  0,  0, 19, 0,  5};

struct item {
  item(const char *) {}
  item(const char *, int8_t s, int8_t e) : start(s), end(e) {}
  int8_t start = -1;
  int8_t end = -1;
};

static const item wordlist[] = {{""},
                                {""},
                                {""},
                                {"age", 20, 20},
                                {"accept", 18, 18},
                                {"expect", 34, 34},
                                {"accept-language", 16, 16},
                                {"host", 37, 37},
                                {"referer", 50, 50},
                                {"accept-charset", 14, 14},
                                {"retry-after", 52, 52},
                                {"refresh", 51, 51},
                                {"content-type", 30, 30},
                                {"content-language", 26, 26},
                                {"content-range", 29, 29},
                                {"location", 45, 45},
                                {"content-length", 27, 27},
                                {"date", 32, 32},
                                {"via", 59, 59},
                                {"authorization", 22, 22},
                                {"range", 49, 49},
                                {":path", 3, 4},
                                {":scheme", 5, 6},
                                {"content-location", 28, 28},
                                {"if-range", 41, 41},
                                {"accept-ranges", 17, 17},
                                {"content-encoding", 25, 25},
                                {"cache-control", 23, 23},
                                {"if-match", 38, 38},
                                {":method", 1, 2},
                                {"accept-encoding", 15, 15},
                                {":authority", 0, 0},
                                {"etag", 33, 33},
                                {"cookie", 31, 31},
                                {"content-disposition", 24, 24},
                                {"last-modified", 43, 43},
                                {"max-forwards", 46, 46},
                                {":status", 7, 13},
                                {"transfer-encoding", 56, 56},
                                {"user-agent", 57, 57},
                                {"if-none-match", 40, 40},
                                {"server", 53, 53},
                                {"expires", 35, 35},
                                {"proxy-authenticate", 47, 47},
                                {"if-modified-since", 39, 39},
                                {"vary", 58, 58},
                                {"www-authenticate", 60, 60},
                                {"access-control-allow-origin", 19, 19},
                                {"link", 44, 44},
                                {"from", 36, 36},
                                "",
                                "",
                                {"set-cookie", 54, 54},
                                {"proxy-authorization", 48, 48},
                                "",
                                "",
                                "",
                                {"strict-transport-security", 55, 55},
                                "",
                                "",
                                "",
                                "",
                                "",
                                {"if-unmodified-since", 42, 42},
                                "",
                                {"allow", 21, 21}};

std::pair<int, int> index(const std::span<const uint8_t> name) {
  enum { TOTAL_KEYWORDS = 52, MIN_WORD_LENGTH = 3, MAX_WORD_LENGTH = 27, MIN_HASH_VALUE = 3, MAX_HASH_VALUE = 65 };

  if (name.size() < MIN_WORD_LENGTH || name.size() > MAX_WORD_LENGTH) {
    return {-1, -1};
  }

  const unsigned char *str = name.data();
  unsigned int hval = 0;

  switch (name.size()) {
  default:
    hval += asso_values[str[8]];
  /*FALLTHROUGH*/
  case 8:
  case 7:
  case 6:
  case 5:
  case 4:
    hval += asso_values[str[3]];
  /*FALLTHROUGH*/
  case 3:
  case 2:
  case 1:
    hval += asso_values[str[0]];
    break;
  }
  auto key = hval + asso_values[str[name.size() - 1]];

  if (key <= MAX_HASH_VALUE && name.size() == lengthtable[key]) {
    return {wordlist[key].start, wordlist[key].end};
  }
  return {-1, -1};
}
} // namespace

namespace rfc7541 {
namespace static_table {

std::size_t size() noexcept { return std::size(field_table); }

std::pair<std::span<const uint8_t>, std::span<const uint8_t>> at(std::size_t index) noexcept {
  const auto &[name, value] = field_table[index - 1];
  return {{reinterpret_cast<const uint8_t *>(name.data()), name.size_bytes()},
          {reinterpret_cast<const uint8_t *>(value.data()), value.size_bytes()}};
}

int name_index(const std::span<const uint8_t> name) noexcept {
  auto [start, end] = index(name);
  if (start < 0 || field_table[start].name != name) {
    return -1;
  }
  return start + 1;
}

std::pair<int, bool> field_index(const std::span<const uint8_t> name, const std::span<const uint8_t> value) noexcept {
  auto [start, end] = index(name);
  if (start < 0 || field_table[start].name != name) {
    return {-1, false};
  }

  // Searching in the value diapason
  auto *end_ptr = &field_table[end + 1];
  if (auto it = std::find_if(&field_table[start], end_ptr,
                             [value](const static_field &field) {
                               return field.value.size_bytes() == value.size_bytes() && field.value == value;
                             });
      it != end_ptr) {
    return {1 + std::distance(&field_table[0], it), true};
  }
  return {++start, false};
}

} // namespace static_table
} // namespace rfc7541
