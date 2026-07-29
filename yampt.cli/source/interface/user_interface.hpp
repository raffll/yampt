#pragma once

#include <utility/domain_types.hpp>
#include <utility/includes.hpp>

class user_interface_t
{
public:
	user_interface_t(std::vector<std::string> & arg);

private:
	void parse_command_line();
	void collect_argument_value(const std::string & command, const std::string & value);
	void run_command();

	void make_dict_();
	void make_dict_base();
	void merge_dict();
	void convert_esm();
	void create_esm();
	void make_loc();

	std::vector<std::string> args;
	std::vector<std::string> file_paths;
	std::vector<std::string> dict_paths;
	std::string output;
	std::string suffix;
	std::string translate_model_path;
	std::string esm_name_override;
	std::string language_code;

	bool partial_mode = false;
};
