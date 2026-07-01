#include "esm_converter.hpp"
#include "script_parser.hpp"
#include "../utility/keyword_trie.hpp"

static bool is_approved_status(status_t status)
{
	return status == status_t::translated;
}

esm_converter_t::esm_converter_t(
    const std::string & path,
    const dict_merger_t & merger,
    const bool add_hyperlinks,
    const std::string & file_suffix,
    const codepage_t encoding,
    const bool create_header)
    : esm(path)
    , merger(merger)
    , add_hyperlinks(add_hyperlinks)
    , file_suffix(file_suffix)
    , create_header(create_header)
{
	if (encoding == codepage_t::windows_1250)
	{
		if (detect_encoding())
		{
			this->add_hyperlinks = false;
		}
	}

	if (esm.is_loaded())
		convert_esm();
}

void esm_converter_t::convert_esm()
{
	if (!file_suffix.empty())
		convert_mast();

	convert_gmst();
	convert_fnam();
	convert_desc();
	convert_text();
	convert_rnam();
	convert_indx();

	if (add_hyperlinks)
	{
		build_hyperlink_trie();
		tools_t::add_log("[info] adding hyperlinks\r\n");
	}

	convert_info();
	convert_bnam();
	convert_scpt();
	convert_cell();
	convert_pgrd();
	convert_anam();
	convert_scvr();
	convert_dnam();
	convert_cndt();
	convert_dial();
	// convert_gmdt();

	if (create_header)
		make_header();
}

void esm_converter_t::reset_counters()
{
	counter_converted = 0;
	counter_identical = 0;
	counter_unchanged = 0;
	counter_all = 0;
	counter_added = 0;
}

bool esm_converter_t::make_new_text(const tools_t::entry_t & entry, std::string & new_text)
{
	counter_all++;
	new_text.clear();

	const auto * found = merger.get_dict().at(entry.type).find(entry.key_text);

	if (found)
	{
		if (!is_approved_status(found->status))
		{
			counter_unchanged++;
			return false;
		}

		new_text = found->new_text;
		if (is_identical(entry.old_text, new_text))
			return false;

		counter_converted++;
		return true;
	}

	counter_unchanged++;
	tools_t::add_log("[warning] unchanged " + tools_t::type_to_str(entry.type) + ": " + entry.key_text + "\r\n", true);
	return false;
}

bool esm_converter_t::is_identical(const std::string & old_text, const std::string & new_text)
{
	if (old_text == new_text)
	{
		counter_identical++;
		return true;
	}

	return false;
}

void esm_converter_t::add_null_terminator_if_empty(std::string & new_text)
{
	if (new_text.empty())
		new_text = '\0';
}

void esm_converter_t::convert_record_content(const std::string & new_text)
{
	std::string rec_content = esm.get_record().content;
	rec_content.erase(esm.get_value().pos + 8, esm.get_value().size);
	rec_content.insert(esm.get_value().pos + 8, new_text);
	rec_content.erase(esm.get_value().pos + 4, 4);
	rec_content.insert(esm.get_value().pos + 4, tools_t::convert_uint_to_string_byte_array(new_text.size()));
	size_t rec_size = rec_content.size() - 16;
	rec_content.erase(4, 4);
	rec_content.insert(4, tools_t::convert_uint_to_string_byte_array(rec_size));
	esm.replace_record(rec_content);
}

void esm_converter_t::print_log_line(const tools_t::rec_type_t type)
{
	std::string line = tools_t::type_to_str(type) + ": converted=" + std::to_string(counter_converted) +
	                   ", identical=" + std::to_string(counter_identical) +
	                   ", unchanged=" + std::to_string(counter_unchanged) + ", total=" + std::to_string(counter_all) +
	                   "\r\n";

	tools_t::add_log(line);
}

void esm_converter_t::build_hyperlink_trie()
{
	const auto & dict = merger.get_dict();
	auto dial_it = dict.find(tools_t::rec_type_t::dial);
	if (dial_it == dict.end())
		return;

	for (const auto & entry : dial_it->second.records)
	{
		if (!is_approved_status(entry.status))
			continue;

		if (entry.new_text.empty())
			continue;

		m_hyperlink_trie.seed(entry.new_text, entry.new_text);
	}
}

std::string esm_converter_t::insert_hyperlink_markers(const std::string & text) const
{
	const auto & matches = m_hyperlink_trie.find_matches(text);
	if (matches.empty())
		return text;

	std::string result;
	result.reserve(text.size() + matches.size());

	size_t last_position = 0;
	for (const auto & match : matches)
	{
		result.append(text, last_position, match.start - last_position);
		result += '@';
		result.append(text, match.start, match.length);
		last_position = match.start + match.length;
	}

	result.append(text, last_position, text.size() - last_position);
	return result;
}

bool esm_converter_t::detect_encoding()
{
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id == "INFO")
			esm.set_value("NAME");

		if (detect_windows_1250_encoding(esm.get_value().text))
		{
			tools_t::add_log("[warning] windows-1250 encoding detected\r\n");
			tools_t::add_log(esm.get_value().text + "\r\n", true);
			return true;
		}
	}
	return false;
}

bool esm_converter_t::detect_windows_1250_encoding(const std::string & text)
{
	std::ostringstream ss;
	ss << static_cast<char>(156) << static_cast<char>(159) << static_cast<char>(179) << static_cast<char>(185)
	   << static_cast<char>(191) << static_cast<char>(230) << static_cast<char>(234) << static_cast<char>(241);

	return text.find_first_of(ss.str()) != std::string::npos;
}
