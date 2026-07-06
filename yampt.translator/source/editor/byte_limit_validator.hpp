#pragma once

#include <io/codepage.hpp>
#include <utility/domain_types.hpp>
#include <string>

enum class validation_level_t
{
	ok,
	caution,
	error
};

struct validation_result_t
{
	validation_level_t level;
	size_t byte_count;
	size_t limit;
};

class byte_limit_validator_t
{
public:
	validation_result_t validate(rec_type_t type, const std::string & utf8_value) const;
	void set_codepage(codepage_t cp);

private:
	codepage_t m_codepage = codepage_t::windows_1252;
};
