#pragma once

#include <cstdint>
#include <string>
#include <vector>

class header_repair_t
{
public:
	static std::string update_master_sizes(const std::string & header_content, const std::string & plugin_directory);

	static std::string update_version_to_1_3(const std::string & header_content);
};
