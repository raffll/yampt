#include "byte_limit_validator.hpp"

void byte_limit_validator_t::set_codepage(codepage_t cp)
{
	m_codepage = cp;
}

codepage_t byte_limit_validator_t::codepage() const
{
	return m_codepage;
}

validation_result_t byte_limit_validator_t::validate(rec_type_t type, const std::string & utf8_value) const
{
	const auto encode_result = encode_from_utf8_checked(utf8_value, m_codepage);
	const size_t byte_count = encode_result.encoded.size();

	if (encode_result.has_unmappable_chars)
		return { validation_level_t::error, byte_count, 0, "contains characters not representable in " + std::string(codepage_name(m_codepage)) };

	for (size_t i = 0; i < utf8_value.size(); ++i)
	{
		unsigned char ch = static_cast<unsigned char>(utf8_value[i]);
		if (ch == '|' || ch == '~' || ch == '@' || ch == '{' || ch == '}')
			return { validation_level_t::error, byte_count, 0, "forbidden character: " + std::string(1, static_cast<char>(ch)) };

		if (ch <= 0x1F && ch != 0x09 && ch != 0x0D && ch != 0x0A)
			return { validation_level_t::error, byte_count, 0, "control character: 0x" + std::string(1, "0123456789ABCDEF"[ch >> 4]) + std::string(1, "0123456789ABCDEF"[ch & 0xF]) };

		if (ch == '"' && (type == rec_type_t::sctx || type == rec_type_t::bnam))
		{
			continue;
		}
	}

	if (type == rec_type_t::cell || type == rec_type_t::dial)
	{
		if (byte_count > 63)
			return { validation_level_t::error, byte_count, 63, "exceeds 63 byte limit" };

		return { validation_level_t::ok, byte_count, 63, {} };
	}

	if (type == rec_type_t::fnam)
	{
		if (byte_count > 31)
			return { validation_level_t::error, byte_count, 31, "exceeds 31 byte limit" };

		return { validation_level_t::ok, byte_count, 31, {} };
	}

	if (type == rec_type_t::rnam)
	{
		if (byte_count > 32)
			return { validation_level_t::error, byte_count, 32, "exceeds 32 byte limit" };

		return { validation_level_t::ok, byte_count, 32, {} };
	}

	if (type == rec_type_t::info)
	{
		if (byte_count > 1024)
			return { validation_level_t::error, byte_count, 1024, "exceeds 1024 byte limit" };

		if (byte_count > 512)
			return { validation_level_t::caution, byte_count, 512, {} };

		return { validation_level_t::ok, byte_count, 512, {} };
	}

	return { validation_level_t::ok, byte_count, 0, {} };
}
