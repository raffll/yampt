#include "dict_creator.hpp"
#include "../utility/string_utils.hpp"
#include "translation_engine.hpp"
#include <hunspell/hunspell.hxx>

dict_creator_t::dict_creator_t(const std::string & plugin_path, const tools_t::dict_t * base_dict)
    : esm(plugin_path)
    , esm_ref(esm)
    , base_dict(base_dict)
    , mode(base_dict ? mode_t::single_with_base : mode_t::single)
{
	dict = tools_t::initialize_dict();

	if (esm.is_loaded())
		make_dict_single();
}

dict_creator_t::dict_creator_t(
    const std::string & path,
    const std::string & path_ext,
    translation_engine_t * translation_engine,
    base_mode_t base_mode)
    : esm(path_ext)
    , esm_ext(path)
    , esm_ref(esm_ext)
    , mode(mode_t::base)
    , translation_engine_(translation_engine)
    , base_mode_(base_mode)
{
	dict = tools_t::initialize_dict();

	if (esm.is_loaded() && esm_ext.is_loaded())
	{
		std::string native_ids;
		native_ids.reserve(esm.get_records().size() * 4);
		for (const auto & rec : esm.get_records())
			native_ids += rec.id;

		std::string foreign_ids;
		foreign_ids.reserve(esm_ext.get_records().size() * 4);
		for (const auto & rec : esm_ext.get_records())
			foreign_ids += rec.id;

		if (native_ids == foreign_ids)
		{
			mode = mode_t::base_ordered;
			tools_t::add_log("[info] record order: identical\r\n");
			make_dict_base_ordered();
		}
		else
		{
			mode = mode_t::base;
			tools_t::add_log("[info] record order: different\r\n");
			make_dict_base();
		}
	}
}

dict_creator_t::~dict_creator_t() = default;

void dict_creator_t::load_english_dict()
{
	if (base_mode_ != base_mode_t::partial)
		return;

	auto dir = tools_t::get_exe_dir();
	if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
		dir += '/';

	auto aff = dir + "dictionaries/en_US.aff";
	auto dic = dir + "dictionaries/en_US.dic";
	english_dict_ = std::make_unique<Hunspell>(aff.c_str(), dic.c_str());
}

void dict_creator_t::build_npc_index()
{
	for (size_t i = 0; i < esm_ref.get_records().size(); ++i)
	{
		esm_ref.select_record(i);
		if (esm_ref.get_record().id != "NPC_")
			continue;

		esm_ref.set_key("NAME");
		if (!esm_ref.get_key().exist)
			continue;

		npc_index.insert({ esm_ref.get_key().text, i });
	}
}

void dict_creator_t::reset_counters()
{
	counter_created = 0;
	counter_missing = 0;
	counter_doubled = 0;
	counter_identical = 0;
	counter_all = 0;
}

static bool is_number_or_punct(char c)
{
	return (c >= '0' && c <= '9');
}

bool dict_creator_t::differs_only_in_numbers_or_punct(const std::string & a, const std::string & b)
{
	if (a.size() != b.size())
		return false;

	bool has_difference = false;

	for (size_t i = 0; i < a.size(); ++i)
	{
		if (a[i] == b[i])
			continue;

		if (!is_number_or_punct(a[i]) || !is_number_or_punct(b[i]))
			return false;

		has_difference = true;
	}

	return has_difference;
}

std::string dict_creator_t::adapt_translation(
    const std::string & source,
    const std::string & matched_source,
    const std::string & matched_translation)
{
	if (source.size() != matched_source.size())
		return matched_translation;

	std::string result = matched_translation;

	for (size_t i = 0; i < source.size() && i < matched_source.size(); ++i)
	{
		if (source[i] == matched_source[i])
			continue;

		size_t search_start = 0;
		auto pos = result.find(matched_source[i], search_start);
		bool replaced = false;

		while (pos != std::string::npos)
		{
			if (is_number_or_punct(result[pos]))
			{
				result[pos] = source[i];
				replaced = true;
				break;
			}

			search_start = pos + 1;
			pos = result.find(matched_source[i], search_start);
		}

		if (!replaced)
		{
			for (size_t j = 0; j < result.size(); ++j)
			{
				if (result[j] != matched_source[i])
					continue;

				result[j] = source[i];
				break;
			}
		}
	}

	return result;
}

void dict_creator_t::insert_duplicate(
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    tools_t::rec_type_t type,
    status_t status)
{
	auto * dup = dict.at(type).find(key_text);
	if (dup)
	{
		dup->status = status_t::duplicate;
		if (dup->details.empty())
			dup->details = new_text;
		else
			dup->details += "|" + new_text;

		counter_doubled++;
	}
}

std::vector<std::string> dict_creator_t::make_script_messages(const std::string & script_text)
{
	std::vector<std::string> messages;
	std::string line;
	std::string line_lc;
	std::istringstream ss(script_text);

	while (std::getline(ss, line))
	{
		line = tools_t::trim_cr(line);
		line_lc = string_utils::to_lower(line);

		size_t keyword_pos;
		std::set<size_t> keyword_pos_coll;

		for (const auto & keyword : tools_t::keywords)
		{
			keyword_pos = line_lc.find(keyword);
			if (keyword_pos == std::string::npos)
				continue;

			if (keyword_pos > 0 &&
			    (std::isalnum(static_cast<unsigned char>(line_lc[keyword_pos - 1])) || line_lc[keyword_pos - 1] == '_'))
				continue;

			if (keyword_pos + keyword.size() < line_lc.size() &&
			    (std::isalnum(static_cast<unsigned char>(line_lc[keyword_pos + keyword.size()])) ||
			     line_lc[keyword_pos + keyword.size()] == '_'))
				continue;

			keyword_pos_coll.insert(keyword_pos);
		}

		if (keyword_pos_coll.empty())
			continue;

		keyword_pos = *keyword_pos_coll.begin();

		if (keyword_pos != std::string::npos && line.rfind(";", keyword_pos) == std::string::npos &&
		    line.find("\"", keyword_pos) != std::string::npos)
		{
			messages.push_back(line);
		}
	}
	return messages;
}
