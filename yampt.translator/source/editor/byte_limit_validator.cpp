#include "byte_limit_validator.hpp"

void byte_limit_validator_t::set_codepage(codepage_t cp)
{
	m_codepage = cp;
}

validation_result_t byte_limit_validator_t::validate(tools_t::rec_type_t type, const std::string & utf8_value) const
{
	const auto encoded = encode_from_utf8(utf8_value, m_codepage);
	const size_t byte_count = encoded.size();

	bool has_forbidden = false;
	for (size_t i = 0; i < utf8_value.size(); ++i)
	{
		unsigned char ch = static_cast<unsigned char>(utf8_value[i]);
		if (ch == '|' || ch == '~' || ch == '@' || ch == '{' || ch == '}')
		{
			has_forbidden = true;
			break;
		}

		if (ch <= 0x1F && ch != 0x09 && ch != 0x0D && ch != 0x0A)
		{
			has_forbidden = true;
			break;
		}

		if (ch == '"' && (type == tools_t::rec_type_t::sctx || type == tools_t::rec_type_t::bnam))
		{
			has_forbidden = true;
			break;
		}
	}

	if (has_forbidden)
		return { validation_level_t::error, byte_count, 0 };

	if (type == tools_t::rec_type_t::cell)
	{
		if (byte_count > 63)
			return { validation_level_t::error, byte_count, 63 };
		return { validation_level_t::ok, byte_count, 63 };
	}

	if (type == tools_t::rec_type_t::fnam)
	{
		if (byte_count > 31)
			return { validation_level_t::error, byte_count, 31 };
		return { validation_level_t::ok, byte_count, 31 };
	}

	if (type == tools_t::rec_type_t::rnam)
	{
		if (byte_count > 32)
			return { validation_level_t::error, byte_count, 32 };
		return { validation_level_t::ok, byte_count, 32 };
	}

	if (type == tools_t::rec_type_t::info)
	{
		if (byte_count > 1024)
			return { validation_level_t::error, byte_count, 1024 };
		if (byte_count > 512)
			return { validation_level_t::caution, byte_count, 512 };
		return { validation_level_t::ok, byte_count, 512 };
	}

	return { validation_level_t::ok, byte_count, 0 };
}
