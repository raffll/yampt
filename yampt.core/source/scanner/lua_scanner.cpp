#include "lua_scanner.hpp"
#include "../utility/string_utils.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

struct scan_context_t
{
	std::string data_path;
	std::string mod_name;
	handler_parser_t handler_parser;
	lua_scan_result_t result;
	const std::atomic<bool> & cancelled;
};

static std::string read_file_content(const std::string & file_path)
{
	std::ifstream stream(file_path);
	if (!stream.is_open())
		return {};

	std::ostringstream buffer;
	buffer << stream.rdbuf();
	return buffer.str();
}

static std::vector<std::string> discover_omwscripts(const std::string & data_path)
{
	std::vector<std::string> omwscripts_files;
	const auto normalized = string_utils::normalize_path(data_path);

	if (!std::filesystem::exists(normalized))
		return omwscripts_files;

	if (!std::filesystem::is_directory(normalized))
		return omwscripts_files;

	for (const auto & entry : std::filesystem::recursive_directory_iterator(normalized))
	{
		if (!entry.is_regular_file())
			continue;

		const auto filename = string_utils::to_lower(entry.path().filename().string());

		if (filename.ends_with(".omwscripts"))
		{
			omwscripts_files.push_back(string_utils::normalize_path(entry.path().string()));
			continue;
		}

		if (!filename.ends_with(".omwscripts.esp"))
			continue;

		const auto real_name = entry.path().filename().string();
		const auto stripped = real_name.substr(0, real_name.size() - 4);
		const auto real_path = entry.path().parent_path() / stripped;

		if (!std::filesystem::exists(real_path))
			continue;

		const auto normalized_real = string_utils::normalize_path(real_path.string());
		const auto already_found = std::find(omwscripts_files.begin(), omwscripts_files.end(), normalized_real);

		if (already_found == omwscripts_files.end())
			omwscripts_files.push_back(normalized_real);
	}

	return omwscripts_files;
}

static std::string resolve_script_path(const std::string & data_path, const std::string & relative_path)
{
	const auto normalized_base = string_utils::normalize_path(data_path);
	const auto normalized_rel = string_utils::normalize_path(relative_path);
	return normalized_base + "/" + normalized_rel;
}

static void parse_single_entry(const omwscripts_entry_t & entry, scan_context_t & context)
{
	const auto full_path = resolve_script_path(context.data_path, entry.script_path);
	const auto content = read_file_content(full_path);

	if (content.empty())
		return;

	handler_parser_t::parse_input_t input;
	input.file_content = content;
	input.script_path = entry.script_path;
	input.mod_name = context.mod_name;

	const auto registrations = context.handler_parser.parse(input);
	for (const auto & registration : registrations)
		context.result.registrations.push_back(registration);
}

static void parse_omwscripts_entries(const omwscripts_result_t & parse_result, scan_context_t & context)
{
	for (const auto & entry : parse_result.entries)
	{
		if (context.cancelled.load())
			return;

		if (entry.script_path.empty())
			continue;

		parse_single_entry(entry, context);
	}
}

static void process_omwscripts_file(const std::string & omwscripts_file, scan_context_t & context)
{
	omwscripts_parser_t omwscripts_parser;
	const auto parse_result = omwscripts_parser.parse(omwscripts_file, context.mod_name);

	for (const auto & warning : parse_result.warnings)
		context.result.warnings.push_back(warning);

	parse_omwscripts_entries(parse_result, context);
}

static void scan_single_data_path(scan_context_t & context)
{
	const auto omwscripts_files = discover_omwscripts(context.data_path);

	for (const auto & omwscripts_file : omwscripts_files)
	{
		if (context.cancelled.load())
			return;

		process_omwscripts_file(omwscripts_file, context);
	}
}

lua_scan_result_t lua_scanner_t::scan(
    const std::vector<std::string> & data_paths,
    const std::vector<std::string> & mod_names)
{
	m_cancelled.store(false);
	scan_context_t context { {}, {}, {}, {}, m_cancelled };

	for (size_t index = 0; index < data_paths.size(); ++index)
	{
		if (m_cancelled.load())
			return {};

		context.data_path = data_paths[index];
		context.mod_name = (index < mod_names.size()) ? mod_names[index] : data_paths[index];

		scan_single_data_path(context);
	}

	if (m_cancelled.load())
		return {};

	conflict_detector_t detector;
	context.result.conflicts = detector.detect(context.result.registrations);

	return context.result;
}

void lua_scanner_t::cancel()
{
	m_cancelled.store(true);
}

bool lua_scanner_t::is_cancelled() const
{
	return m_cancelled.load();
}
