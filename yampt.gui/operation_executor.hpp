#pragma once

#include "../yampt/tools.hpp"
#include <string>
#include <vector>

class operation_executor_t
{
public:
	struct result_t
	{
		bool success = false;
		std::string log_text;
		std::string output_path;
	};

	result_t make_dict(const std::string & plugin_path, tools_t::encoding_t encoding);
	result_t make_dict_with_base(const std::string & plugin_path, const tools_t::dict_t & base_dict, tools_t::encoding_t encoding);
	result_t make_base(const std::string & foreign_path, const std::string & native_path);
	result_t convert(const std::string & plugin_path, const std::vector<std::string> & dict_paths, tools_t::encoding_t encoding);
	result_t create_plugin(const std::string & plugin_path, const std::vector<std::string> & dict_paths, tools_t::encoding_t encoding);

private:
	std::string make_output_path(const std::string & source_path, const std::string & ext) const;
	std::string get_workspace_dir() const;
};
