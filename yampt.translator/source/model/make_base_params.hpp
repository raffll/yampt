#pragma once

#include <creator/dict_creator.hpp>
#include <string>

struct make_base_params_t
{
	std::string native_path;
	std::string foreign_lang;
	std::string native_lang;
	base_mode_t base_mode;
	std::string dictionary_aff_path;
};
