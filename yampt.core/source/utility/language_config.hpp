#pragma once

#include "../io/codepage.hpp"
#include <string>
#include <vector>

struct language_entry_t
{
	std::string code;
	std::string display_name;
	std::string nllb_code;
	std::string dictionary_prefix;
	codepage_t codepage = codepage_t::windows_1252;
};

namespace language_config {

std::vector<language_entry_t> load(const std::string & json_path);

const language_entry_t * find_by_code(const std::vector<language_entry_t> & languages, const std::string & code);

codepage_t resolve_codepage(const std::vector<language_entry_t> & languages, const std::string & code);

std::string resolve_dictionary_prefix(const std::vector<language_entry_t> & languages, const std::string & code);

} // namespace language_config
