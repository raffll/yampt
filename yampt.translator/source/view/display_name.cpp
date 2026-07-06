#include "display_name.hpp"

display_name_t::display_name_t(const std::string & filename)
    : m_filename(filename)
{}

void display_name_t::set_filename(const std::string & filename)
{
	m_filename = filename;
}

void display_name_t::set_kind(dict_kind_t kind)
{
	m_kind = kind;
}

void display_name_t::set_file_type(file_type_t type)
{
	m_file_type = type;
}

void display_name_t::set_language(const std::string & lang)
{
	m_language = lang;
}

void display_name_t::set_dirty(bool dirty)
{
	m_dirty = dirty;
}

void display_name_t::set_wip(bool wip)
{
	m_wip = wip;
}

void display_name_t::set_unloaded(bool unloaded)
{
	m_unloaded = unloaded;
}

std::string display_name_t::to_string() const
{
	std::string result;

	if (m_dirty)
		result += "* ";

	if (m_unloaded)
		result += "[UNLOADED] ";

	if (m_kind == dict_kind_t::base)
		result += "[BASE] ";

	if (m_wip)
		result += "[WIP] ";

	if (!m_language.empty())
		result += "[" + m_language + "] ";

	result += m_filename;
	return result;
}

const std::string & display_name_t::filename() const
{
	return m_filename;
}
