#pragma once

#include <string>
#include <vector>

enum class handler_class_t
{
	blocking,
	mutating,
	passive
};

struct handler_registration_t
{
	std::string interface_name;
	std::string method_name;
	std::string type_argument;
	std::string callback_expression;
	std::string handler_body;
	std::string blocking_condition;
	handler_class_t classification = handler_class_t::passive;
	std::string script_path;
	int line_number = 0;
	std::string mod_name;
};

class handler_parser_t
{
public:
	struct parse_input_t
	{
		std::string file_content;
		std::string script_path;
		std::string mod_name;
	};

	std::vector<handler_registration_t> parse(const parse_input_t & input);
};
