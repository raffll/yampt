#pragma once

#include <string>
#include <vector>

static constexpr int gui_rec_type_yaml = 100;

struct l10n_entry_t
{
	std::string key;
	std::string value;
};

class yaml_l10n_reader_t
{
public:
	yaml_l10n_reader_t() = default;

	bool load(const std::string & source_path, const std::string & target_path = "");

	const std::vector<l10n_entry_t> & source_entries() const;
	const std::vector<l10n_entry_t> & target_entries() const;
	const std::vector<std::string> & key_order() const;

private:
	std::vector<l10n_entry_t> parse_yaml(const std::string & path);

	std::vector<l10n_entry_t> source_entries_;
	std::vector<l10n_entry_t> target_entries_;
	std::vector<std::string> key_order_;
};
