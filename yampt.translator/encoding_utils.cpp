#include "../yampt/codepage.hpp"
#define NOMINMAX
#include <Windows.h>

std::string decode_to_utf8(const std::string & raw_bytes, codepage_t codepage)
{
	if (raw_bytes.empty())
		return {};

	int cp = static_cast<int>(codepage);

	int wide_len = MultiByteToWideChar(cp, 0, raw_bytes.data(), static_cast<int>(raw_bytes.size()), nullptr, 0);
	if (wide_len <= 0)
		return raw_bytes;

	std::wstring wide(wide_len, L'\0');
	MultiByteToWideChar(cp, 0, raw_bytes.data(), static_cast<int>(raw_bytes.size()), wide.data(), wide_len);

	int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_len, nullptr, 0, nullptr, nullptr);
	if (utf8_len <= 0)
		return raw_bytes;

	std::string utf8(utf8_len, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide_len, utf8.data(), utf8_len, nullptr, nullptr);

	return utf8;
}

std::string encode_from_utf8(const std::string & utf8_text, codepage_t codepage)
{
	if (utf8_text.empty())
		return {};

	int cp = static_cast<int>(codepage);

	int wide_len = MultiByteToWideChar(CP_UTF8, 0, utf8_text.data(), static_cast<int>(utf8_text.size()), nullptr, 0);
	if (wide_len <= 0)
		return utf8_text;

	std::wstring wide(wide_len, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, utf8_text.data(), static_cast<int>(utf8_text.size()), wide.data(), wide_len);

	int mb_len = WideCharToMultiByte(cp, 0, wide.data(), wide_len, nullptr, 0, nullptr, nullptr);
	if (mb_len <= 0)
		return utf8_text;

	std::string result(mb_len, '\0');
	WideCharToMultiByte(cp, 0, wide.data(), wide_len, result.data(), mb_len, nullptr, nullptr);

	return result;
}

const char * codepage_name(codepage_t cp)
{
	switch (cp)
	{
	case codepage_t::windows_1250:
		return "Windows-1250 (Central European)";
	case codepage_t::windows_1251:
		return "Windows-1251 (Cyrillic)";
	case codepage_t::windows_1252:
		return "Windows-1252 (Western)";
	}
	return "Unknown";
}
