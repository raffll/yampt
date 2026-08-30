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

struct encode_result_t
{
	std::string encoded;
	bool has_unmappable_chars = false;
};

encode_result_t encode_from_utf8_checked(const std::string & utf8_text, codepage_t codepage);

const char * codepage_name(codepage_t cp);

constexpr codepage_t supported_codepages[] = {
	codepage_t::windows_1250,
	codepage_t::windows_1251,
	codepage_t::windows_1252,
};

constexpr int codepage_to_index(codepage_t codepage)
{
	switch (codepage)
	{
	case codepage_t::windows_1250:
		return 0;
	case codepage_t::windows_1251:
		return 1;
	case codepage_t::windows_1252:
		return 2;
	}
	return 2;
}

constexpr codepage_t index_to_codepage(int index)
{
	switch (index)
	{
	case 0:
		return codepage_t::windows_1250;
	case 1:
		return codepage_t::windows_1251;
	case 2:
		return codepage_t::windows_1252;
	}
	return codepage_t::windows_1252;
}
