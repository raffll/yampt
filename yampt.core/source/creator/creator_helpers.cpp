#include "creator_helpers.hpp"
#include "../utility/app_logger.hpp"
#include "../utility/string_utils.hpp"
#include <hunspell/hunspell.hxx>
#include <sstream>

creator_context_t::~creator_context_t() = default;

static bool is_number_or_punct(char c)
{
	return (c >= '0' && c <= '9');
}

void creator_helpers_t::insert_entry_single(
    creator_context_t & ctx,
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    rec_type_t type)
{
	ctx.counter_all++;

	const bool is_text_keyed =
	    (type == rec_type_t::cell || type == rec_type_t::dial || type == rec_type_t::sctx ||
	     type == rec_type_t::bnam);

	auto * existing = ctx.dict.at(type).find(key_text);
	if (existing && is_text_keyed)
	{
		if (existing->old_text == old_text && existing->new_text == new_text)
			return;
	}

	record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;
	entry.new_text = new_text;
	entry.status = status_t::untranslated;

	if (ctx.dict.at(type).insert(entry))
	{
		ctx.counter_created++;
		return;
	}

	insert_duplicate(ctx, key_text, old_text, new_text, type, status_t::untranslated);
}

void creator_helpers_t::insert_entry_base(
    creator_context_t & ctx,
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    rec_type_t type,
    status_t status)
{
	ctx.counter_all++;

	const bool is_text_keyed =
	    (type == rec_type_t::cell || type == rec_type_t::dial || type == rec_type_t::sctx ||
	     type == rec_type_t::bnam);

	auto * existing = ctx.dict.at(type).find(key_text);
	if (existing && is_text_keyed)
	{
		if (existing->old_text == old_text && existing->new_text == new_text)
			return;
	}

	status_t final_status = status;
	if (status == status_t::translated)
		final_status = determine_status(ctx, old_text, new_text);

	record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;
	entry.new_text = new_text;
	entry.status = final_status;

	if (ctx.dict.at(type).insert(entry))
	{
		ctx.counter_created++;
		return;
	}

	insert_duplicate(ctx, key_text, old_text, new_text, type, final_status);
}

void creator_helpers_t::insert_duplicate(
    creator_context_t & ctx,
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    rec_type_t type,
    status_t status)
{
	auto * dup = ctx.dict.at(type).find(key_text);
	if (!dup)
		return;

	if (dup->old_text == old_text && dup->new_text == new_text)
		return;

	if (dup->new_text == new_text)
		return;

	dup->status = status_t::duplicate;
	if (dup->details.empty())
		dup->details = new_text;
	else
		dup->details += "|" + new_text;

	ctx.counter_doubled++;
}

void creator_helpers_t::insert_as_untranslated(
    creator_context_t & ctx,
    const std::string & key_text,
    const std::string & old_text,
    rec_type_t type)
{
	record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;
	entry.new_text = old_text;
	entry.status = status_t::untranslated;

	if (ctx.dict.at(type).insert(entry))
	{
		ctx.counter_created++;
		return;
	}

	insert_duplicate(ctx, key_text, old_text, old_text, type, status_t::untranslated);
}

void creator_helpers_t::insert_with_status(
    creator_context_t & ctx,
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    rec_type_t type,
    status_t status)
{
	record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;
	entry.new_text = new_text;
	entry.status = status;

	if (ctx.dict.at(type).insert(entry))
	{
		ctx.counter_created++;
		return;
	}

	insert_duplicate(ctx, key_text, old_text, new_text, type, status);
}

void creator_helpers_t::insert_via_text_match(
    creator_context_t & ctx,
    const std::string & key_text,
    const std::string & old_text,
    rec_type_t type)
{
	const auto outcome = ctx.text_match_index.find(old_text);

	if (outcome.result == text_match_index_t::find_result_t::found)
	{
		creator_helpers_t::insert_with_status(ctx, key_text, old_text, outcome.translation, type, status_t::reused);
		return;
	}

	if (outcome.result == text_match_index_t::find_result_t::ambiguous)
	{
		creator_helpers_t::insert_with_status(ctx, key_text, old_text, outcome.translation, type, status_t::ambiguous);

		auto * entry = ctx.dict.at(type).find(key_text);
		if (entry)
			entry->details = outcome.conflicts;

		return;
	}

	insert_as_untranslated(ctx, key_text, old_text, type);
}

void creator_helpers_t::insert_changed_entry(
    creator_context_t & ctx,
    const std::string & key_text,
    const std::string & old_text,
    const record_entry_t & base_entry,
    rec_type_t type)
{
	const auto & base_status = base_entry.status;
	const bool is_approved = (base_status == status_t::translated);

	if (!is_approved)
	{
		insert_unapproved_changed(ctx, key_text, old_text, base_entry, type);
		return;
	}

	if (differs_only_in_numbers_or_punct(old_text, base_entry.old_text))
	{
		insert_adapted_entry(ctx, key_text, old_text, base_entry, type);
		return;
	}

	creator_helpers_t::insert_with_status(ctx, key_text, old_text, base_entry.new_text, type, status_t::changed);

	auto * changed_entry = ctx.dict.at(type).find(key_text);
	if (changed_entry)
		changed_entry->details = base_entry.old_text;
}

void creator_helpers_t::insert_unapproved_changed(
    creator_context_t & ctx,
    const std::string & key_text,
    const std::string & old_text,
    const record_entry_t & base_entry,
    rec_type_t type)
{
	const auto & base_status = base_entry.status;

	if (base_status == status_t::in_progress || base_status == status_t::model || base_status == status_t::error)
	{
		creator_helpers_t::insert_with_status(ctx, key_text, old_text, base_entry.new_text, type, status_t::outdated);

		auto * outdated_entry = ctx.dict.at(type).find(key_text);
		if (outdated_entry)
			outdated_entry->details = base_entry.old_text;

		return;
	}

	if (base_status == status_t::untranslated)
	{
		creator_helpers_t::insert_with_status(ctx, key_text, old_text, old_text, type, status_t::untranslated);
		return;
	}

	creator_helpers_t::insert_with_status(ctx, key_text, old_text, old_text, type, status_t::changed);

	auto * changed_entry = ctx.dict.at(type).find(key_text);
	if (changed_entry)
		changed_entry->details = base_entry.old_text;
}

void creator_helpers_t::insert_adapted_entry(
    creator_context_t & ctx,
    const std::string & key_text,
    const std::string & old_text,
    const record_entry_t & base_entry,
    rec_type_t type)
{
	const auto & adapted = creator_helpers_t::adapt_translation(old_text, base_entry.old_text, base_entry.new_text);

	if (adapted == base_entry.new_text)
	{
		creator_helpers_t::insert_with_status(ctx, key_text, old_text, adapted, type, status_t::translated);
		return;
	}

	creator_helpers_t::insert_with_status(ctx, key_text, old_text, adapted, type, status_t::adapted);

	auto * entry = ctx.dict.at(type).find(key_text);
	if (entry)
		entry->details = base_entry.new_text;
}

void creator_helpers_t::insert_entry_single_with_base(
    creator_context_t & ctx,
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    rec_type_t type)
{
	ctx.counter_all++;

	const bool is_text_keyed =
	    (type == rec_type_t::cell || type == rec_type_t::dial || type == rec_type_t::sctx ||
	     type == rec_type_t::bnam);

	auto * existing = ctx.dict.at(type).find(key_text);
	if (existing && is_text_keyed)
	{
		if (existing->old_text == old_text && existing->new_text == new_text)
			return;
	}

	auto it_chapter = ctx.base_dict->find(type);
	if (it_chapter == ctx.base_dict->end())
	{
		insert_as_untranslated(ctx, key_text, old_text, type);
		return;
	}

	const auto * base_entry = it_chapter->second.find(key_text);
	if (!base_entry)
	{
		insert_via_text_match(ctx, key_text, old_text, type);
		return;
	}

	if (base_entry->status == status_t::mismatch)
	{
		creator_helpers_t::insert_with_status(ctx, key_text, old_text, base_entry->new_text, type, status_t::mismatch);
		return;
	}

	if (base_entry->old_text == old_text && base_entry->new_text == old_text)
	{
		creator_helpers_t::insert_with_status(ctx, key_text, old_text, old_text, type, base_entry->status);
		return;
	}

	if (base_entry->old_text == old_text)
	{
		const auto & base_status = base_entry->status;
		const bool preserve =
		    (base_status == status_t::untranslated || base_status == status_t::to_verify ||
		     base_status == status_t::in_progress || base_status == status_t::model ||
		     base_status == status_t::propagated || base_status == status_t::error ||
		     base_status == status_t::translated);
		const auto status = preserve ? base_status : status_t::translated;
		creator_helpers_t::insert_with_status(ctx, key_text, old_text, base_entry->new_text, type, status);
		return;
	}

	insert_changed_entry(ctx, key_text, old_text, *base_entry, type);
}

std::vector<std::string> creator_helpers_t::make_script_messages(const std::string & script_text)
{
	std::vector<std::string> messages;
	std::string line;
	std::string line_lc;
	std::istringstream stream(script_text);

	while (std::getline(stream, line))
	{
		line = string_utils::trim_cr(line);
		line_lc = string_utils::to_lower(line);

		size_t keyword_pos;
		std::set<size_t> keyword_pos_coll;

		for (const auto & keyword : domain_types_t::script_keywords)
		{
			keyword_pos = line_lc.find(keyword);
			if (keyword_pos == std::string::npos)
				continue;

			if (keyword_pos > 0 &&
			    (std::isalnum(static_cast<unsigned char>(line_lc[keyword_pos - 1])) ||
			     line_lc[keyword_pos - 1] == '_'))
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

void creator_helpers_t::build_npc_index(creator_context_t & ctx)
{
	for (size_t i = 0; i < ctx.esm_ref.get_records().size(); ++i)
	{
		ctx.esm_ref.select_record(i);
		if (ctx.esm_ref.get_record().id != "NPC_")
			continue;

		ctx.esm_ref.set_key("NAME");
		if (!ctx.esm_ref.get_key().exist)
			continue;

		ctx.npc_index.insert({ ctx.esm_ref.get_key().text, i });
	}
}

void creator_helpers_t::build_text_match_index(creator_context_t & ctx)
{
	if (!ctx.base_dict)
		return;

	ctx.text_match_index.build(*ctx.base_dict);
}

void creator_helpers_t::build_gmst_index(creator_context_t & ctx)
{
	for (size_t i = 0; i < ctx.esm_ref.get_records().size(); ++i)
	{
		ctx.esm_ref.select_record(i);
		if (ctx.esm_ref.get_record().id != "GMST")
			continue;

		ctx.esm_ref.set_key("NAME");
		if (!ctx.esm_ref.get_key().exist)
			continue;

		if (ctx.esm_ref.get_key().text.substr(0, 1) != "s")
			continue;

		ctx.gmst_index.insert({ ctx.esm_ref.get_key().text, i });
	}
}

void creator_helpers_t::build_fnam_index(creator_context_t & ctx)
{
	for (size_t i = 0; i < ctx.esm_ref.get_records().size(); ++i)
	{
		ctx.esm_ref.select_record(i);
		if (!domain_types_t::is_fnam(ctx.esm_ref.get_record().id))
			continue;

		ctx.esm_ref.set_key("NAME");
		if (!ctx.esm_ref.get_key().exist)
			continue;

		if (ctx.esm_ref.get_key().text == "player")
			continue;

		const auto key = ctx.esm_ref.get_record().id + "^" + ctx.esm_ref.get_key().text;
		ctx.fnam_index.insert({ key, i });
	}
}

void creator_helpers_t::build_desc_index(creator_context_t & ctx)
{
	for (size_t i = 0; i < ctx.esm_ref.get_records().size(); ++i)
	{
		ctx.esm_ref.select_record(i);
		const auto & rec_id = ctx.esm_ref.get_record().id;
		if (rec_id != "BSGN" && rec_id != "CLAS" && rec_id != "RACE")
			continue;

		ctx.esm_ref.set_key("NAME");
		if (!ctx.esm_ref.get_key().exist)
			continue;

		const auto key = rec_id + "^" + ctx.esm_ref.get_key().text;
		ctx.desc_index.insert({ key, i });
	}
}

void creator_helpers_t::build_text_index(creator_context_t & ctx)
{
	for (size_t i = 0; i < ctx.esm_ref.get_records().size(); ++i)
	{
		ctx.esm_ref.select_record(i);
		if (ctx.esm_ref.get_record().id != "BOOK")
			continue;

		ctx.esm_ref.set_key("NAME");
		if (!ctx.esm_ref.get_key().exist)
			continue;

		ctx.text_index.insert({ ctx.esm_ref.get_key().text, i });
	}
}

void creator_helpers_t::build_rnam_index(creator_context_t & ctx)
{
	for (size_t i = 0; i < ctx.esm_ref.get_records().size(); ++i)
	{
		ctx.esm_ref.select_record(i);
		if (ctx.esm_ref.get_record().id != "FACT")
			continue;

		ctx.esm_ref.set_key("NAME");
		if (!ctx.esm_ref.get_key().exist)
			continue;

		ctx.esm_ref.set_value("RNAM");
		while (ctx.esm_ref.get_value().exist)
		{
			const auto key = ctx.esm_ref.get_key().text + "^" +
			                 std::to_string(ctx.esm_ref.get_value().counter);
			ctx.rnam_index.insert({ key, ctx.esm_ref.get_value().text });
			ctx.esm_ref.set_next_value("RNAM");
		}
	}
}

void creator_helpers_t::build_indx_index(creator_context_t & ctx)
{
	for (size_t i = 0; i < ctx.esm_ref.get_records().size(); ++i)
	{
		ctx.esm_ref.select_record(i);
		const auto & rec_id = ctx.esm_ref.get_record().id;
		if (rec_id != "SKIL" && rec_id != "MGEF")
			continue;

		ctx.esm_ref.set_key("INDX");
		if (!ctx.esm_ref.get_key().exist)
			continue;

		const auto key = rec_id + "^" + domain_types_t::get_indx(ctx.esm_ref.get_key().content);
		ctx.indx_index.insert({ key, i });
	}
}

void creator_helpers_t::build_info_index(creator_context_t & ctx)
{
	std::string dial_prefix;
	for (size_t i = 0; i < ctx.esm_ref.get_records().size(); ++i)
	{
		ctx.esm_ref.select_record(i);
		if (ctx.esm_ref.get_record().id == "DIAL")
		{
			ctx.esm_ref.set_key("DATA");
			ctx.esm_ref.set_value("NAME");
			if (ctx.esm_ref.get_key().exist && ctx.esm_ref.get_value().exist)
				dial_prefix = domain_types_t::get_dialog_type(ctx.esm_ref.get_key().content) + "^" +
				              ctx.esm_ref.get_value().text;

			continue;
		}

		if (ctx.esm_ref.get_record().id != "INFO")
			continue;

		ctx.esm_ref.set_key("INAM");
		if (!ctx.esm_ref.get_key().exist)
			continue;

		const auto key = dial_prefix + "^" + ctx.esm_ref.get_key().text;
		ctx.info_index.insert({ key, i });
	}
}

void creator_helpers_t::load_english_dict(creator_context_t & ctx)
{
	if (ctx.base_mode != base_mode_t::partial)
		return;

	std::string aff;
	std::string dic;

	if (!ctx.dictionary_aff_path.empty())
	{
		aff = ctx.dictionary_aff_path;
		dic = ctx.dictionary_aff_path;
		auto dot_pos = dic.rfind(".aff");
		if (dot_pos != std::string::npos)
			dic.replace(dot_pos, 4, ".dic");
	}
	else
	{
		auto dir = app_logger_t::get_exe_dir();
		if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
			dir += '/';

		aff = dir + "dictionaries/en_US.aff";
		dic = dir + "dictionaries/en_US.dic";
	}

	ctx.english_dict = std::make_unique<Hunspell>(aff.c_str(), dic.c_str());
}

status_t creator_helpers_t::determine_status(
    const creator_context_t & ctx,
    const std::string & old_text,
    const std::string & new_text)
{
	if (old_text != new_text)
		return status_t::translated;

	if (ctx.base_mode == base_mode_t::full)
		return status_t::translated;

	if (is_proper_noun(ctx, old_text))
		return status_t::to_verify;

	return status_t::untranslated;
}

bool creator_helpers_t::is_proper_noun(const creator_context_t & ctx, const std::string & text)
{
	if (!ctx.english_dict)
		return true;

	std::string token;
	for (const auto ch : text)
	{
		if (std::isalnum(static_cast<unsigned char>(ch)))
		{
			token += ch;
			continue;
		}

		if (token.size() >= 3 && ctx.english_dict->spell(token))
			return false;

		token.clear();
	}

	if (token.size() >= 3 && ctx.english_dict->spell(token))
		return false;

	return true;
}

bool creator_helpers_t::differs_only_in_numbers_or_punct(const std::string & a, const std::string & b)
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

std::string creator_helpers_t::adapt_translation(
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

void creator_helpers_t::enrich_info_speaker(
    creator_context_t & ctx,
    const std::string & key_text,
    size_t record_index)
{
	ctx.esm.select_record(record_index);
	ctx.esm.set_value("ONAM");
	if (!ctx.esm.get_value().exist || ctx.esm.get_value().text.empty())
		return;

	const auto & speaker_id = ctx.esm.get_value().text;
	auto npc_search = ctx.npc_index.find(speaker_id);
	if (npc_search == ctx.npc_index.end())
		return;

	ctx.esm.select_record(npc_search->second);
	ctx.esm.set_key("FNAM");
	ctx.esm.set_value("FLAG");

	std::string speaker_name;
	if (ctx.esm.get_key().exist)
		speaker_name = ctx.esm.get_key().text;

	std::string gender;
	if (ctx.esm.get_value().exist)
		gender = ((domain_types_t::convert_string_byte_array_to_uint(ctx.esm.get_value().content) & 0x0001) != 0) ? "F" : "M";

	auto * entry = ctx.dict.at(rec_type_t::info).find(key_text);
	if (!entry)
		return;

	entry->speaker_name = speaker_name;
	entry->gender = gender;
}
