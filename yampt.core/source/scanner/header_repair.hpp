#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct header_master_t
{
	std::string filename;
	uint64_t file_size = 0;
};

class header_repair_t
{
public:
	static std::string update_master_sizes(const std::string & header_content, const std::string & plugin_directory);

	static std::string update_version_to_1_3(const std::string & header_content);

	static std::optional<std::string> update_record_count(const std::string & header_content, size_t record_count);

	static std::optional<std::string> append_missing_masters(
	    const std::string & header_content,
	    const std::vector<header_master_t> & masters);
};
