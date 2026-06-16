#pragma once

#include <string>
#include <vector>

struct l10n_entry_t;

class yaml_l10n_writer_t
{
public:
	yaml_l10n_writer_t() = default;

	bool write(const std::string & output_path,
	           const std::vector<l10n_entry_t> & entries,
	           const std::vector<std::string> & key_order);
};
