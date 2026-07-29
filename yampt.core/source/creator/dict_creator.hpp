#pragma once

#include "creator_context.hpp"
#include "creator_helpers.hpp"

class dict_creator_t
{
public:
	dict_creator_t(const std::string & plugin_path, const dict_t * base_dict = nullptr);

	dict_creator_t(
	    const std::string & path,
	    const std::string & path_ext,
	    translation_engine_t * translation_engine = nullptr,
	    base_mode_t base_mode = base_mode_t::full,
	    const std::string & dictionary_aff_path = {});

	~dict_creator_t();

	const auto & get_name()
	{
		return m_ctx.esm.get_name();
	}

	const auto & get_dict() const
	{
		return m_ctx.dict;
	}

	static bool differs_only_in_numbers_or_punct(const std::string & a, const std::string & b)
	{
		return creator_helpers::differs_only_in_numbers_or_punct(a, b);
	}

	static std::string adapt_translation(
	    const std::string & source,
	    const std::string & matched_source,
	    const std::string & matched_translation)
	{
		return creator_helpers::adapt_translation(source, matched_source, matched_translation);
	}

private:
	creator_context_t m_ctx;
};
