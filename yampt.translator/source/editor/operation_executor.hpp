#pragma once

#include "../model/plugin_op.hpp"
#include <creator/dict_creator.hpp>
#include <io/codepage.hpp>
#include <utility/domain_types.hpp>
#include <string>
#include <vector>

class translation_engine_t;

class operation_executor_t
{
public:
	struct result_t
	{
		bool success = false;
		std::string log_text;
		std::string output_path;
	};

	void set_output_dir(const std::string & dir)
	{
		m_output_dir = dir;
	}

	result_t make_dict(const std::string & plugin_path, codepage_t encoding);
	result_t make_dict_with_base(
	    const std::string & plugin_path,
	    const dict_t & base_dict,
	    codepage_t encoding);
	result_t make_base(
	    const std::string & foreign_path,
	    const std::string & native_path,
	    const std::string & foreign_lang = {},
	    const std::string & native_lang = {},
	    translation_engine_t * engine = nullptr,
	    base_mode_t mode = base_mode_t::full,
	    const std::string & dictionary_aff_path = {});
	result_t convert(const std::string & plugin_path, const std::vector<std::string> & dict_paths, codepage_t encoding);
	result_t create_plugin(
	    const std::string & plugin_path,
	    const std::vector<std::string> & dict_paths,
	    codepage_t encoding);

private:
	std::string make_output_path(const std::string & source_path, const std::string & ext) const;
	std::string make_output_path(const std::string & base_name, const std::string & ext, bool) const;
	std::string get_output_dir() const;

	std::string m_output_dir;
};
