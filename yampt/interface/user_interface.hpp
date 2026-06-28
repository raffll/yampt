#pragma once

#include "../utility/includes.hpp"
#include "../io/codepage.hpp"
#include "../utility/tools.hpp"

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
	std::string translate_model_path;

	codepage_t encoding = codepage_t::windows_1252;
	bool partial_mode = false;
};
