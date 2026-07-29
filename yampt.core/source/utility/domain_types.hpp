#pragma once

#include "status_types.hpp"
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

struct file_path_parts_t
{
	std::string full;
	std::string name;
	std::string ext;

	void set_name(const std::string & path)
	{
		full = path.substr(path.find_last_of("\\/") + 1);
		name = full.substr(0, full.find_last_of("."));
		ext = full.substr(full.rfind("."));
	}
};

enum class rec_type_t
{
	cell,
	dial,
	indx,
	rnam,
	desc,
	gmst,
	fnam,
	info,
	text,
	bnam,
	sctx,

	script,

	yaml,

	pgrd,
	anam,
	scvr,
	dnam,
	cndt,
	gmdt,

	wild,
	regn,

	unknown,
};

enum class creator_mode_t
{
	base
};

struct record_entry_t
{
	std::string key_text;
	std::string old_text;
	std::string new_text;
	status_t status = status_t::untranslated;
	std::string speaker_name;
	std::string gender;
	std::string enchantment;
	std::string details;
};

struct chapter_t
{
	std::vector<record_entry_t> records;
	std::unordered_map<std::string, size_t> index;
	std::unordered_map<std::string, size_t> old_text_index;

	bool insert(const record_entry_t & entry);
	record_entry_t * find(const std::string & id);
	const record_entry_t * find(const std::string & id) const;
	record_entry_t * find_by_old_text(const std::string & old_text);
	const record_entry_t * find_by_old_text(const std::string & old_text) const;

	size_t size() const
	{
		return records.size();
	}

	bool empty() const
	{
		return records.empty();
	}
};

using dict_t = std::map<rec_type_t, chapter_t>;

struct entry_t
{
	const std::string key_text;
	std::string old_text;
	const rec_type_t type;
	const std::string optional = "";
};

struct record_t
{
	const std::string id;
	std::string content;
	size_t size = 0;
	bool modified = false;
};

namespace domain_types {

extern const std::vector<std::string> script_keywords;

dict_t initialize_dict();
size_t get_number_of_elements_in_dict(const dict_t & dict);
std::string type_to_str(rec_type_t type);
rec_type_t str_to_type(const std::string & str);
std::string get_dialog_type(const std::string & content);
std::string get_indx(const std::string & content);
bool is_fnam(const std::string & rec_id);
size_t convert_string_byte_array_to_uint(const std::string & str);
std::string convert_uint_to_string_byte_array(size_t size);

} // namespace domain_types
