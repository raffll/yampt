#pragma once

#include <model/dict_kind.hpp>
#include <model/file_list.hpp>
#include <string>

class display_name_t
{
public:
	display_name_t() = default;
	explicit display_name_t(const std::string & filename);

	void set_filename(const std::string & filename);
	void set_kind(dict_kind_t kind);
	void set_file_type(file_type_t type);
	void set_language(const std::string & lang);
	void set_dirty(bool dirty);
	void set_wip(bool wip);
	void set_unloaded(bool unloaded);

	std::string to_string() const;
	const std::string & filename() const;

private:
	std::string filename_;
	dict_kind_t kind_ = dict_kind_t::user;
	file_type_t file_type_ = file_type_t::user_dict;
	std::string language_;
	bool dirty_ = false;
	bool wip_ = false;
	bool unloaded_ = false;
};
