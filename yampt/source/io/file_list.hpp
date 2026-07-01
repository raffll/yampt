#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum class file_type_t
{
	plugin,
	base_dict,
	user_dict,
	yaml_l10n
};

struct file_entry_t
{
	std::string path;
	std::string filename;
	file_type_t type;
	std::string language_tag;
	bool is_workspace = false;
	std::string workspace_subfolder;
	std::string root_path;
};

class file_list_t
{
public:
	const file_entry_t * get(const std::string & path) const;
	file_entry_t * get(const std::string & path);
	bool contains(const std::string & path) const;
	std::vector<const file_entry_t *> all() const;
	std::vector<const file_entry_t *> workspace_files() const;

	file_entry_t & add(const std::string & path);
	void remove(const std::string & path);

	void scan_roots(const std::vector<std::string> & root_paths);
	const std::vector<std::string> & get_roots() const;
	void clear_workspace();

	static file_type_t classify(const std::string & path);
	static std::string detect_language(const std::string & filename, std::uintmax_t file_size);

private:
	void scan_single_root(const std::string & root_path);
	std::unordered_map<std::string, file_entry_t> m_entries;
	std::vector<std::string> m_roots;
};

file_type_t classify(const std::string & path);
std::string detect_language(const std::string & filename, std::uintmax_t file_size);
