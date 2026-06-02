#pragma once

#include "includes.hpp"
#include "tools.hpp"

class user_interface_t
{
public:
	user_interface_t(std::vector<std::string> & arg);

private:
	void parse_command_line();
	void run_command();

	void make_dict_();
	void make_dict_base();
	void merge_dict();
	void convert_esm();
	void create_esm();

	std::vector<std::string> args;
	std::vector<std::string> file_paths;
	std::vector<std::string> dict_paths;
	std::string output;
	std::string suffix;

	tools_t::encoding_t encoding = tools_t::encoding_t::unknown;
};
