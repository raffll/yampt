#include "tools.hpp"

std::string tools_t::log1;
bool tools_t::error_flag = false;

const std::vector<std::string> tools_t::keywords { "messagebox", "choice", "say" };

bool tools_t::chapter_t::insert(const record_entry_t & entry)
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

tools_t::record_entry_t * tools_t::chapter_t::find(const std::string & id)
{
	auto it = index.find(id);
	if (it == index.end())
		return nullptr;
	return &records[it->second];
}

const tools_t::record_entry_t * tools_t::chapter_t::find(const std::string & id) const
{
	auto it = index.find(id);
	if (it == index.end())
		return nullptr;
	return &records[it->second];
}

tools_t::record_entry_t * tools_t::chapter_t::find_by_old_text(const std::string & old_text)
{
	auto it = old_text_index.find(old_text);
	if (it == old_text_index.end())
		return nullptr;
	return &records[it->second];
}

const tools_t::record_entry_t * tools_t::chapter_t::find_by_old_text(const std::string & old_text) const
{
	auto it = old_text_index.find(old_text);
	if (it == old_text_index.end())
		return nullptr;
	return &records[it->second];
}

std::string tools_t::read_file(const std::string & path)
{
	std::string content;
	std::ifstream file(path, std::ios::binary);
	if (file)
	{
		add_log("[info] loading \"" + path + "\"\r\n");
		char buffer[16384];
		file.seekg(0, std::ios::end);
		content.reserve(static_cast<size_t>(file.tellg()));
		file.seekg(0, std::ios::beg);
		std::streamsize chars_read;
		while (file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
		{
			content.append(buffer, chars_read);
		}
	}
	else
	{
		add_log("[error] cannot load \"" + path + "\" (wrong path)\r\n");
	}
	return content;
}

void tools_t::write_text(const std::string & text, const std::string & name)
{
	std::ofstream file(name, std::ios::binary);
	file << text;
	add_log("[info] writing \"" + name + "\"\r\n");
}

void tools_t::write_file(const std::vector<record_t> & records, const std::string & name)
{
	std::ofstream file(name, std::ios::binary);
	for (const auto & record : records)
	{
		file << record.content;
	}
	add_log("[info] writing \"" + name + "\"\r\n");
}

void tools_t::create_file(const std::vector<record_t> & records, const std::string & name)
{
	std::ofstream file(name, std::ios::binary);
	for (size_t i = 0; i < records.size(); ++i)
	{
		const auto & record = records.at(i);
		if (!record.modified)
			continue;
		else
			file << record.content;
	}
	add_log("[info] writing \"" + name + "\"\r\n");
}

size_t tools_t::get_number_of_elements_in_dict(const dict_t & dict)
{
	size_t size = 0;
	for (const auto & chapter : dict)
	{
		size += chapter.second.size();
	}
	return size;
}

size_t tools_t::convert_string_byte_array_to_uint(const std::string & str)
{
	assert(str.size() == 4 || str.size() == 1);

	char buffer[4] = {};
	unsigned char ubuffer[4];
	unsigned int x;
	str.copy(buffer, str.size());
	for (int i = 0; i < 4; i++)
	{
		ubuffer[i] = buffer[i];
	}

	if (str.size() == 4)
	{
		return x = (ubuffer[0] | ubuffer[1] << 8 | ubuffer[2] << 16 | ubuffer[3] << 24);
	}

	if (str.size() == 1)
	{
		return x = ubuffer[0];
	}

	return std::string::npos;
}

std::string tools_t::convert_uint_to_string_byte_array(const size_t size)
{
	auto x = static_cast<const unsigned>(size);

	char bytes[4];
	std::string str;
	std::copy(
	    static_cast<const char *>(static_cast<const void *>(&x)),
	    static_cast<const char *>(static_cast<const void *>(&x)) + sizeof x,
	    bytes);
	for (int i = 0; i < 4; i++)
	{
		str.push_back(bytes[i]);
	}
	return str;
}

bool tools_t::case_insensitive_string_cmp(std::string lhs, std::string rhs)
{
	transform(lhs.begin(), lhs.end(), lhs.begin(), ::toupper);
	transform(rhs.begin(), rhs.end(), rhs.begin(), ::toupper);
	return lhs == rhs;
}

std::string tools_t::erase_null_chars(std::string str)
{
	size_t is_null = str.find('\0');
	if (is_null != std::string::npos)
	{
		str.erase(is_null);
	}
	return str;
}

std::string tools_t::trim_cr(std::string str)
{
	if (!str.empty() && str.back() == '\r')
	{
		str.pop_back();
	}
	return str;
}

std::string tools_t::replace_non_readable_chars_with_dot(const std::string & str)
{
	std::string text;
	for (size_t i = 0; i < str.size(); ++i)
	{
		if (std::isprint(static_cast<unsigned char>(str[i])))
		{
			text += str[i];
		}
		else
		{
			text += ".";
		}
	}
	return text;
}

void tools_t::add_log(const std::string & entry, const bool silent)
{
	if (entry.find("[error]") == 0)
	{
		error_flag = true;
	}

	std::cout << entry;
	log1 += entry;
}

void tools_t::reset_log()
{
	log1.clear();
	error_flag = false;
}

tools_t::dict_t tools_t::initialize_dict()
{
	return {
		{ tools_t::rec_type_t::cell, {} }, { tools_t::rec_type_t::dial, {} }, { tools_t::rec_type_t::indx, {} },
		{ tools_t::rec_type_t::rnam, {} }, { tools_t::rec_type_t::desc, {} }, { tools_t::rec_type_t::gmst, {} },
		{ tools_t::rec_type_t::fnam, {} }, { tools_t::rec_type_t::info, {} }, { tools_t::rec_type_t::text, {} },
		{ tools_t::rec_type_t::bnam, {} }, { tools_t::rec_type_t::sctx, {} },
	};
}

std::string tools_t::type_to_str(tools_t::rec_type_t type)
{
	switch (type)
	{
	case tools_t::rec_type_t::cell:
		return "CELL";
	case tools_t::rec_type_t::dial:
		return "DIAL";
	case tools_t::rec_type_t::indx:
		return "INDX";
	case tools_t::rec_type_t::rnam:
		return "RNAM";
	case tools_t::rec_type_t::desc:
		return "DESC";
	case tools_t::rec_type_t::gmst:
		return "GMST";
	case tools_t::rec_type_t::fnam:
		return "FNAM";
	case tools_t::rec_type_t::info:
		return "INFO";
	case tools_t::rec_type_t::text:
		return "TEXT";
	case tools_t::rec_type_t::bnam:
		return "BNAM";
	case tools_t::rec_type_t::sctx:
		return "SCTX";

	case tools_t::rec_type_t::pgrd:
		return "PGRD";
	case tools_t::rec_type_t::anam:
		return "ANAM";
	case tools_t::rec_type_t::scvr:
		return "SCVR";
	case tools_t::rec_type_t::dnam:
		return "DNAM";
	case tools_t::rec_type_t::cndt:
		return "CNDT";
	case tools_t::rec_type_t::gmdt:
		return "GMDT";

	case tools_t::rec_type_t::default_val:
		return "+ Default";
	case tools_t::rec_type_t::regn:
		return "+ REGN";

	default:
		return "N/A";
	}
}

tools_t::rec_type_t tools_t::str_to_type(const std::string & str)
{
	std::map<std::string, tools_t::rec_type_t> str2type {
		{ "CELL", tools_t::rec_type_t::cell }, { "DIAL", tools_t::rec_type_t::dial },
		{ "INDX", tools_t::rec_type_t::indx }, { "RNAM", tools_t::rec_type_t::rnam },
		{ "DESC", tools_t::rec_type_t::desc }, { "GMST", tools_t::rec_type_t::gmst },
		{ "FNAM", tools_t::rec_type_t::fnam }, { "INFO", tools_t::rec_type_t::info },
		{ "TEXT", tools_t::rec_type_t::text }, { "BNAM", tools_t::rec_type_t::bnam },
		{ "SCTX", tools_t::rec_type_t::sctx },
	};

	auto search = str2type.find(str);
	if (search != str2type.end())
		return search->second;
	else
		return tools_t::rec_type_t::unknown;
}

std::string tools_t::get_dialog_type(const std::string & content)
{
	static const std::vector<std::string> dialog_type { "T", "V", "G", "P", "J" };
	size_t type = tools_t::convert_string_byte_array_to_uint(content.substr(0, 1));
	return dialog_type.at(type);
}

std::string tools_t::get_indx(const std::string & content)
{
	size_t indx = tools_t::convert_string_byte_array_to_uint(content);
	std::ostringstream ss;
	ss << std::setfill('0') << std::setw(3) << indx;
	return ss.str();
}

bool tools_t::is_fnam(const std::string & rec_id)
{
	return (
	    rec_id == "ACTI" || rec_id == "ALCH" || rec_id == "APPA" || rec_id == "ARMO" || rec_id == "BOOK" ||
	    rec_id == "BSGN" || rec_id == "CLAS" || rec_id == "CLOT" || rec_id == "CONT" || rec_id == "CREA" ||
	    rec_id == "DOOR" || rec_id == "FACT" || rec_id == "INGR" || rec_id == "LIGH" || rec_id == "LOCK" ||
	    rec_id == "MISC" || rec_id == "NPC_" || rec_id == "PROB" || rec_id == "RACE" || rec_id == "REGN" ||
	    rec_id == "REPA" || rec_id == "SKIL" || rec_id == "SPEL" || rec_id == "WEAP");
}
