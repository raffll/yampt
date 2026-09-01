#pragma once

#include "glossary.hpp"
#include <io/codepage.hpp>
#include <io/loc_types.hpp>
#include <string>
#include <vector>

class inflection_store_t
{
public:
	void rebuild(const std::vector<std::string> & loc_paths, codepage_t codepage);
	void clear();

	std::vector<annotation_t> annotate(const std::string & translation_text) const;

private:
	std::vector<loc_types::loc_entry_t> m_entries;
};
