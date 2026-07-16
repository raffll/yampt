#include "domain_types.hpp"
#include "record_types.hpp"
#include "string_utils.hpp"
#include <cassert>
#include <iomanip>
#include <sstream>

static constexpr size_t bytes_per_uint32 = 4;

const std::vector<std::string> domain_types::script_keywords { "messagebox", "choice", "say" };

bool chapter_t::insert(const record_entry_t & entry)
{
	auto it = index.find(entry.key_text);
	if (it != index.end())
		return false;

	size_t pos = records.size();
	index[entry.key_text] = pos;
	records.push_back(entry);

	if (!entry.old_text.empty())
		old_text_index.insert({ entry.old_text, pos });

	return true;
}

record_entry_t * chapter_t::find(const std::string & id)
{
	auto it = index.find(id);
	if (it == index.end())
		return nullptr;
	return &records[it->second];
}

const record_entry_t * chapter_t::find(const std::string & id) const
{
	auto it = index.find(id);
	if (it == index.end())
		return nullptr;
	return &records[it->second];
}

record_entry_t * chapter_t::find_by_old_text(const std::string & old_text)
{
	auto it = old_text_index.find(old_text);
	if (it == old_text_index.end())
		return nullptr;
	return &records[it->second];
}

const record_entry_t * chapter_t::find_by_old_text(const std::string & old_text) const
{
	auto it = old_text_index.find(old_text);
	if (it == old_text_index.end())
		return nullptr;
	return &records[it->second];
}

dict_t domain_types::initialize_dict()
{
	return {
		{ rec_type_t::cell, {} }, { rec_type_t::dial, {} }, { rec_type_t::indx, {} }, { rec_type_t::rnam, {} },
		{ rec_type_t::desc, {} }, { rec_type_t::gmst, {} }, { rec_type_t::fnam, {} }, { rec_type_t::info, {} },
		{ rec_type_t::text, {} }, { rec_type_t::bnam, {} }, { rec_type_t::sctx, {} }, { rec_type_t::script, {} },
	};
}

size_t domain_types::get_number_of_elements_in_dict(const dict_t & dict)
{
	size_t total = 0;
	for (const auto & chapter : dict)
	{
		total += chapter.second.size();
	}
	return total;
}

std::string domain_types::type_to_str(rec_type_t type)
{
	switch (type)
	{
	case rec_type_t::cell:
		return "CELL";
	case rec_type_t::dial:
		return "DIAL";
	case rec_type_t::indx:
		return "INDX";
	case rec_type_t::rnam:
		return "RNAM";
	case rec_type_t::desc:
		return "DESC";
	case rec_type_t::gmst:
		return "GMST";
	case rec_type_t::fnam:
		return "FNAM";
	case rec_type_t::info:
		return "INFO";
	case rec_type_t::text:
		return "TEXT";
	case rec_type_t::bnam:
		return "BNAM";
	case rec_type_t::sctx:
		return "SCTX";
	case rec_type_t::script:
		return "SCRIPT";

	case rec_type_t::pgrd:
		return "PGRD";
	case rec_type_t::anam:
		return "ANAM";
	case rec_type_t::scvr:
		return "SCVR";
	case rec_type_t::dnam:
		return "DNAM";
	case rec_type_t::cndt:
		return "CNDT";
	case rec_type_t::gmdt:
		return "GMDT";

	case rec_type_t::wild:
		return "CELL";
	case rec_type_t::regn:
		return "REGN";

	default:
		return "N/A";
	}
}

rec_type_t domain_types::str_to_type(const std::string & str)
{
	static const std::map<std::string, rec_type_t> str2type {
		{ "CELL", rec_type_t::cell }, { "DIAL", rec_type_t::dial }, { "INDX", rec_type_t::indx },
		{ "RNAM", rec_type_t::rnam }, { "DESC", rec_type_t::desc }, { "GMST", rec_type_t::gmst },
		{ "FNAM", rec_type_t::fnam }, { "INFO", rec_type_t::info }, { "TEXT", rec_type_t::text },
		{ "BNAM", rec_type_t::bnam }, { "SCTX", rec_type_t::sctx },
		{ "SCRIPT", rec_type_t::script },
	};

	auto search = str2type.find(str);
	if (search != str2type.end())
		return search->second;

	return rec_type_t::unknown;
}

std::string domain_types::get_dialog_type(const std::string & content)
{
	static const std::vector<std::string> dialog_type { "T", "V", "G", "P", "J" };
	size_t type = convert_string_byte_array_to_uint(content.substr(0, 1));
	return dialog_type.at(type);
}

std::string domain_types::get_indx(const std::string & content)
{
	size_t indx = convert_string_byte_array_to_uint(content);
	std::ostringstream stream;
	stream << std::setfill('0') << std::setw(3) << indx;
	return stream.str();
}

bool domain_types::is_fnam(const std::string & rec_id)
{
	return record_types::is_fnam_eligible(rec_id);
}

size_t domain_types::convert_string_byte_array_to_uint(const std::string & str)
{
	assert(str.size() == bytes_per_uint32 || str.size() == 1);

	char buffer[bytes_per_uint32] = {};
	unsigned char ubuffer[bytes_per_uint32];
	unsigned int result_value;
	str.copy(buffer, str.size());
	for (size_t i = 0; i < bytes_per_uint32; i++)
	{
		ubuffer[i] = buffer[i];
	}

	if (str.size() == bytes_per_uint32)
	{
		return result_value = (ubuffer[0] | ubuffer[1] << 8 | ubuffer[2] << 16 | ubuffer[3] << 24);
	}

	if (str.size() == 1)
	{
		return result_value = ubuffer[0];
	}

	return std::string::npos;
}

std::string domain_types::convert_uint_to_string_byte_array(size_t size)
{
	auto value = static_cast<const unsigned>(size);

	char bytes[bytes_per_uint32];
	std::string str;
	std::copy(
	    static_cast<const char *>(static_cast<const void *>(&value)),
	    static_cast<const char *>(static_cast<const void *>(&value)) + sizeof value,
	    bytes);
	for (size_t i = 0; i < bytes_per_uint32; i++)
	{
		str.push_back(bytes[i]);
	}
	return str;
}
