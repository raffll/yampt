#pragma once

#include <string>

enum class codepage_t
{
	windows_1250 = 1250,
	windows_1251 = 1251,
	windows_1252 = 1252,
};

std::string decode_to_utf8(const std::string & raw_bytes, codepage_t codepage);

std::string encode_from_utf8(const std::string & utf8_text, codepage_t codepage);

const char * codepage_name(codepage_t cp);

constexpr codepage_t supported_codepages[] = {
	codepage_t::windows_1250,
	codepage_t::windows_1251,
	codepage_t::windows_1252,
};
