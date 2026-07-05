#pragma once

#include "../io/esm_reader.hpp"
#include <string>
#include <unordered_map>
#include <vector>

struct indexed_record_t
{
	std::string rec_type;
	std::string record_id;
	std::string display_name;
	std::string dial_name;
	size_t record_index;
	bool has_dele = false;
};

class plugin_index_t
{
public:
	explicit plugin_index_t(esm_reader_t & esm);

	const std::vector<indexed_record_t> & entries() const;
	const indexed_record_t * find(const std::string & type, const std::string & id) const;
	std::vector<std::string> types() const;
	size_t count_by_type(const std::string & type) const;

private:
	std::string derive_id(esm_reader_t & esm, size_t i);
	std::string derive_display_name(esm_reader_t & esm, size_t i);

	std::vector<indexed_record_t> m_entries;
	std::unordered_map<std::string, size_t> m_lookup;
};
