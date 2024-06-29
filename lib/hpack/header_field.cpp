#include "header_field.h"

namespace rfc7541 {
header_field::~header_field() {
  if (m_type == index_type::NEVER_INDEX) {
    std::fill(m_name.begin(), m_name.end(), 0);
    std::fill(m_value.begin(), m_value.end(), 0);
  }
}
} // namespace rfc7541
