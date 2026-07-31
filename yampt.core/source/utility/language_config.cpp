#include "language_config.hpp"
#include "../io/binary_file_io.hpp"
#include "../io/json_reader.hpp"
#include "app_logger.hpp"

std::vector<language_entry_t> language_config::load(const std::string & json_path)
{
	std::vector<language_entry_t> result;

	const auto content = binary_file_io::read_file(json_path);
	if (content.empty())
	{
		app_logger_t::add_log("[error] cannot read languages file: " + json_path + "\r\n");
		return result;
	}

	json_reader_t reader;
	if (!reader.parse(content.data(), content.size(), false))
	{
		app_logger_t::add_log("[error] cannot parse languages file: " + json_path + "\r\n");
		return result;
	}

	auto * root = reader.root();
	if (!yyjson_is_arr(root))
	{
		app_logger_t::add_log("[error] languages file root must be a JSON array\r\n");
		return result;
	}

	json_reader_t::foreach_arr(
	    root,
	    [&](size_t, yyjson_val * entry)
	{
		if (!yyjson_is_obj(entry))
			return;

		language_entry_t language;
		language.code = json_reader_t::get_string(entry, "code");
		language.display_name = json_reader_t::get_string(entry, "name");
		language.nllb_code = json_reader_t::get_string(entry, "nllb");
		language.dictionary_prefix = json_reader_t::get_string(entry, "dictionary");

		const auto codepage_val = yyjson_obj_get(entry, "codepage");
		if (codepage_val && yyjson_is_int(codepage_val))
			language.codepage = static_cast<codepage_t>(yyjson_get_int(codepage_val));

		if (!language.code.empty())
			result.push_back(std::move(language));
	});

	return result;
}

const language_entry_t * language_config::find_by_code(
    const std::vector<language_entry_t> & languages,
    const std::string & code)
{
	for (const auto & entry : languages)
	{
		if (entry.code == code)
			return &entry;
	}
	return nullptr;
}

codepage_t language_config::resolve_codepage(const std::vector<language_entry_t> & languages, const std::string & code)
{
	const auto * entry = find_by_code(languages, code);
	if (entry)
		return entry->codepage;

	return codepage_t::windows_1252;
}

std::string language_config::resolve_dictionary_prefix(
    const std::vector<language_entry_t> & languages,
    const std::string & code)
{
	const auto * entry = find_by_code(languages, code);
	if (entry)
		return entry->dictionary_prefix;

	return "";
}
