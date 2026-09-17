#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace record_header_flags
{
constexpr size_t flags_offset = 12;
constexpr size_t flags_length = 4;

constexpr uint32_t persistent = 0x00000400;
constexpr uint32_t blocked = 0x00002000;

uint32_t read_flags(const std::string & content);
uint32_t merge_flags(const std::vector<std::string> & versions);
void write_flags(std::string & content, uint32_t flags);
} // namespace record_header_flags
