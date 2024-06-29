#pragma once

#include <cstdint>
#include <span>
#include <utility>

namespace rfc7541::static_table {

std::size_t size() noexcept;

std::pair<std::span<const uint8_t>, std::span<const uint8_t>> at(std::size_t index) noexcept;

int name_index(const std::span<const uint8_t> name) noexcept;
std::pair<int, bool> field_index(const std::span<const uint8_t> name, const std::span<const uint8_t> value) noexcept;

} // namespace rfc7541::static_table
