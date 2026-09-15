#pragma once

#include <cstddef>
#include <string>

struct scvr_condition_t
{
	bool valid = false;
	int index = 0;
	std::string type_name;
	std::string function_chars;
	std::string function_name;
	std::string operator_symbol;
	std::string variable_name;
};

#include <vector>

scvr_condition_t parse_scvr_condition(const char * data, size_t size);

std::string format_scvr_condition(const scvr_condition_t & condition, const std::string & value_text);

const char * scvr_operator_symbol(char comparison_char);

const char * scvr_function_name(int function_index);

std::string scvr_type_name(char type_char);

char scvr_type_char(const std::string & type_name);

char scvr_operator_char(const std::string & operator_symbol);

const std::vector<std::string> & scvr_type_names();

const std::vector<std::string> & scvr_operator_symbols();

const char * scvr_variable_storage_name(char storage_char);

bool scvr_type_uses_function(const std::string & type_name);

bool scvr_type_uses_variable_storage(const std::string & type_name);

std::string scvr_subject_display(const char * data, size_t size);

std::string scvr_subject_label(const char * data, size_t size);
