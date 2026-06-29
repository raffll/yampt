#include "file_list.hpp"
#include "../utility/string_utils.hpp"
#include <algorithm>
#include <filesystem>

static std::string extract_extension(const std::string & path)
{
	const auto filename = string_utils::extract_filename(path);
	const auto pos = filename.rfind('.');
	if (pos == std::string_view::npos)
		return "";

	return string_utils::to_lower(filename.substr(pos));
}

const file_entry_t * file_list_t::get(const std::string & path) const
{
	const auto it = entries_.find(string_utils::normalize_path(path));
	if (it == entries_.end())
		return nullptr;

	return &it->second;
}

file_entry_t * file_list_t::get(const std::string & path)
{
	auto it = entries_.find(string_utils::normalize_path(path));
	if (it == entries_.end())
		return nullptr;

	return &it->second;
}

bool file_list_t::contains(const std::string & path) const
{
	return entries_.count(string_utils::normalize_path(path)) > 0;
}

std::vector<const file_entry_t *> file_list_t::all() const
{
	std::vector<const file_entry_t *> result;
	result.reserve(entries_.size());
	for (const auto & [key, entry] : entries_)
		result.push_back(&entry);

	return result;
}

std::vector<const file_entry_t *> file_list_t::workspace_files() const
{
	std::vector<const file_entry_t *> result;
	for (const auto & [key, entry] : entries_)
		result.push_back(&entry);

	return result;
}

file_entry_t & file_list_t::add(const std::string & path)
{
	const auto normalized = string_utils::normalize_path(path);
	auto it = entries_.find(normalized);
	if (it != entries_.end())
		return it->second;

	file_entry_t entry;
	entry.path = normalized;
	entry.filename = std::string(string_utils::extract_filename(normalized));
	entry.type = classify(normalized);

	auto [inserted_it, success] = entries_.emplace(normalized, std::move(entry));
	return inserted_it->second;
}

void file_list_t::remove(const std::string & path)
{
	entries_.erase(string_utils::normalize_path(path));
}

file_type_t file_list_t::classify(const std::string & path)
{
	const auto ext = extract_extension(path);
	if (ext == ".esm" || ext == ".esp" || ext == ".omwgame" || ext == ".omwaddon")
		return file_type_t::plugin;

	if (ext == ".yaml")
		return file_type_t::yaml_l10n;

	if (ext == ".json" || ext == ".xml")
	{
		const auto filename = string_utils::to_lower(string_utils::extract_filename(path));
		if (filename.find("_base_") != std::string::npos)
			return file_type_t::base_dict;

		return file_type_t::user_dict;
	}

	return file_type_t::user_dict;
}

std::string file_list_t::detect_language(const std::string & filename, std::uintmax_t file_size)
{
	const auto lower = string_utils::to_lower(filename);

	if (lower != "morrowind.esm" && lower != "tribunal.esm" && lower != "bloodmoon.esm")
		return "";

	if (file_size == 79837557 || file_size == 9631798 || file_size == 4565686)
		return "EN";
	if (file_size == 80640776 || file_size == 9797295 || file_size == 6069165)
		return "DE";
	if (file_size == 80105097 || file_size == 9658076 || file_size == 4626565)
		return "PL";
	if (file_size == 80681814 || file_size == 10015689 || file_size == 4697358)
		return "FR";
	if (file_size == 79857000 || file_size == 9702000 || file_size == 4625000)
		return "RU";

	return "";
}

file_type_t classify(const std::string & path)
{
	const auto ext = extract_extension(path);
	if (ext == ".esm" || ext == ".esp" || ext == ".omwgame" || ext == ".omwaddon")
		return file_type_t::plugin;

	if (ext == ".yaml")
		return file_type_t::yaml_l10n;

	if (ext == ".json" || ext == ".xml")
	{
		const auto filename = string_utils::to_lower(string_utils::extract_filename(path));
		if (filename.find("_base_") != std::string::npos)
			return file_type_t::base_dict;

		return file_type_t::user_dict;
	}

	return file_type_t::user_dict;
}

std::string detect_language(const std::string & filename, std::uintmax_t file_size)
{
	const auto lower = string_utils::to_lower(filename);

	if (lower == "morrowind.esm")
	{
		if (file_size == 79837557)
			return "EN";
		if (file_size == 80640776)
			return "DE";
		if (file_size == 80105097)
			return "PL";
		if (file_size == 80681814)
			return "FR";
		if (file_size == 79857000)
			return "RU";
		return "";
	}

	if (lower == "tribunal.esm")
	{
		if (file_size == 9631798)
			return "EN";
		if (file_size == 9797295)
			return "DE";
		if (file_size == 9658076)
			return "PL";
		if (file_size == 10015689)
			return "FR";
		if (file_size == 9702000)
			return "RU";
		return "";
	}

	if (lower == "bloodmoon.esm")
	{
		if (file_size == 4565686)
			return "EN";
		if (file_size == 6069165)
			return "DE";
		if (file_size == 4626565)
			return "PL";
		if (file_size == 4697358)
			return "FR";
		if (file_size == 4625000)
			return "RU";
		return "";
	}

	return "";
}

void file_list_t::clear_workspace()
{
	for (auto it = entries_.begin(); it != entries_.end();)
	{
		if (it->second.is_workspace)
			it = entries_.erase(it);
		else
			++it;
	}
}

void file_list_t::scan_roots(const std::vector<std::string> & root_paths)
{
	clear_workspace();
	roots_ = root_paths;

	for (const auto & root : root_paths)
		scan_single_root(root);
}

const std::vector<std::string> & file_list_t::get_roots() const
{
	return roots_;
}

void file_list_t::scan_single_root(const std::string & root_path)
{
	std::error_code ec;
	if (!std::filesystem::is_directory(root_path, ec))
		return;

	const auto root_fs = std::filesystem::path(root_path);
	const auto normalized_root = string_utils::normalize_path(root_path);

	for (const auto & entry : std::filesystem::recursive_directory_iterator(root_path, ec))
	{
		if (!entry.is_regular_file(ec))
			continue;

		const auto ext = string_utils::to_lower(entry.path().extension().string());
		if (ext != ".esm" && ext != ".esp" && ext != ".json" && ext != ".xml" && ext != ".yaml" && ext != ".omwaddon" &&
		    ext != ".omwgame")
			continue;

		if (ext == ".yaml")
		{
			const auto path_lower = string_utils::to_lower(entry.path().string());
			if (path_lower.find("/l10n/") == std::string::npos && path_lower.find("\\l10n\\") == std::string::npos)
				continue;
		}

		const auto path = entry.path().string();
		auto & fe = add(path);
		fe.is_workspace = true;
		fe.root_path = normalized_root;

		const auto parent = entry.path().parent_path();
		const auto relative = std::filesystem::relative(parent, root_fs, ec);
		if (!ec && relative != ".")
			fe.workspace_subfolder = relative.string();
		else
			fe.workspace_subfolder = "";

		if (fe.type == file_type_t::plugin)
		{
			const auto size = entry.file_size(ec);
			if (!ec)
				fe.language_tag = detect_language(fe.filename, size);
		}
	}
}
