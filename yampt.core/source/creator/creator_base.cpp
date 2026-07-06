#include "../translator/translation_engine.hpp"
#include "../utility/app_logger.hpp"
#include "cell_matcher.hpp"
#include "dial_matcher.hpp"
#include "creator_base.hpp"
#include "creator_helpers.hpp"
#include "word_match_utils.hpp"
#include <hunspell/hunspell.hxx>

creator_base_t::creator_base_t(creator_context_t & context)
    : m_ctx(context)
{
}

void creator_base_t::run()
{
	creator_helpers_t::load_english_dict(m_ctx);

	creator_helpers_t::build_gmst_index(m_ctx);
	creator_helpers_t::build_fnam_index(m_ctx);
	creator_helpers_t::build_desc_index(m_ctx);
	creator_helpers_t::build_text_index(m_ctx);
	creator_helpers_t::build_rnam_index(m_ctx);
	creator_helpers_t::build_indx_index(m_ctx);
	creator_helpers_t::build_npc_index(m_ctx);
	creator_helpers_t::build_info_index(m_ctx);

	make_gmst();
	make_fnam();
	make_desc();
	make_text();
	make_rnam();
	make_indx();
	make_dial();
	make_info();
	make_sctx();
	make_bnam();
	make_cell();
}

void creator_base_t::make_gmst()
{
	m_ctx.reset_counters();
	std::set<std::string> matched_keys;

	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		if (m_ctx.esm.get_record().id != "GMST")
			continue;

		m_ctx.esm.set_key("NAME");
		m_ctx.esm.set_value("STRV");
		if (!m_ctx.esm.get_key().exist || !m_ctx.esm.get_value().exist)
			continue;

		if (m_ctx.esm.get_key().text.substr(0, 1) != "s")
			continue;

		const auto & key_text = m_ctx.esm.get_key().text;
		const auto & new_text = m_ctx.esm.get_value().text;

		auto search = m_ctx.gmst_index.find(key_text);
		if (search != m_ctx.gmst_index.end())
		{
			matched_keys.insert(key_text);
			m_ctx.esm_ref.select_record(search->second);
			m_ctx.esm_ref.set_key("NAME");
			m_ctx.esm_ref.set_value("STRV");
			const auto & old_text = m_ctx.esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, rec_type_t::gmst, status_t::translated);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, rec_type_t::gmst, status_t::mismatch);
		}
	}

	for (const auto & [key, rec_idx] : m_ctx.gmst_index)
	{
		if (matched_keys.count(key))
			continue;

		m_ctx.esm_ref.select_record(rec_idx);
		m_ctx.esm_ref.set_value("STRV");
		if (!m_ctx.esm_ref.get_value().exist)
			continue;

		const auto & old_text = m_ctx.esm_ref.get_value().text;
		insert_entry_base(key, old_text, old_text, rec_type_t::gmst, status_t::missing);
	}
}

void creator_base_t::make_fnam()
{
	m_ctx.reset_counters();
	std::set<std::string> matched_keys;

	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		if (!domain_types_t::is_fnam(m_ctx.esm.get_record().id))
			continue;

		m_ctx.esm.set_key("NAME");
		m_ctx.esm.set_value("FNAM");
		if (!m_ctx.esm.get_key().exist || !m_ctx.esm.get_value().exist)
			continue;

		if (m_ctx.esm.get_key().text == "player")
			continue;

		if (m_ctx.esm.get_value().text.empty())
			continue;

		const auto key_text = m_ctx.esm.get_record().id + "^" + m_ctx.esm.get_key().text;
		const auto & new_text = m_ctx.esm.get_value().text;

		auto search = m_ctx.fnam_index.find(key_text);
		if (search != m_ctx.fnam_index.end())
		{
			matched_keys.insert(key_text);
			m_ctx.esm_ref.select_record(search->second);
			m_ctx.esm_ref.set_key("NAME");
			m_ctx.esm_ref.set_value("FNAM");
			const auto & old_text = m_ctx.esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, rec_type_t::fnam, status_t::translated);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, rec_type_t::fnam, status_t::mismatch);
		}
	}

	for (const auto & [key, rec_idx] : m_ctx.fnam_index)
	{
		if (matched_keys.count(key))
			continue;

		m_ctx.esm_ref.select_record(rec_idx);
		m_ctx.esm_ref.set_value("FNAM");
		if (!m_ctx.esm_ref.get_value().exist || m_ctx.esm_ref.get_value().text.empty())
			continue;

		const auto & old_text = m_ctx.esm_ref.get_value().text;
		insert_entry_base(key, old_text, old_text, rec_type_t::fnam, status_t::missing);
	}
}

void creator_base_t::make_desc()
{
	m_ctx.reset_counters();
	std::set<std::string> matched_keys;

	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		const auto & rec_id = m_ctx.esm.get_record().id;
		if (rec_id != "BSGN" && rec_id != "CLAS" && rec_id != "RACE")
			continue;

		m_ctx.esm.set_key("NAME");
		m_ctx.esm.set_value("DESC");
		if (!m_ctx.esm.get_key().exist || !m_ctx.esm.get_value().exist)
			continue;

		const auto key_text = rec_id + "^" + m_ctx.esm.get_key().text;
		const auto & new_text = m_ctx.esm.get_value().text;

		auto search = m_ctx.desc_index.find(key_text);
		if (search != m_ctx.desc_index.end())
		{
			matched_keys.insert(key_text);
			m_ctx.esm_ref.select_record(search->second);
			m_ctx.esm_ref.set_key("NAME");
			m_ctx.esm_ref.set_value("DESC");
			const auto & old_text = m_ctx.esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, rec_type_t::desc, status_t::translated);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, rec_type_t::desc, status_t::mismatch);
		}
	}

	for (const auto & [key, rec_idx] : m_ctx.desc_index)
	{
		if (matched_keys.count(key))
			continue;

		m_ctx.esm_ref.select_record(rec_idx);
		m_ctx.esm_ref.set_value("DESC");
		if (!m_ctx.esm_ref.get_value().exist)
			continue;

		const auto & old_text = m_ctx.esm_ref.get_value().text;
		insert_entry_base(key, old_text, old_text, rec_type_t::desc, status_t::missing);
	}
}

void creator_base_t::make_text()
{
	m_ctx.reset_counters();
	std::set<std::string> matched_keys;

	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		if (m_ctx.esm.get_record().id != "BOOK")
			continue;

		m_ctx.esm.set_key("NAME");
		m_ctx.esm.set_value("TEXT");
		if (!m_ctx.esm.get_key().exist || !m_ctx.esm.get_value().exist)
			continue;

		const auto & key_text = m_ctx.esm.get_key().text;
		const auto & new_text = m_ctx.esm.get_value().text;

		auto search = m_ctx.text_index.find(key_text);
		if (search != m_ctx.text_index.end())
		{
			matched_keys.insert(key_text);
			m_ctx.esm_ref.select_record(search->second);
			m_ctx.esm_ref.set_key("NAME");
			m_ctx.esm_ref.set_value("TEXT");
			const auto & old_text = m_ctx.esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, rec_type_t::text, status_t::translated);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, rec_type_t::text, status_t::mismatch);
		}
	}

	for (const auto & [key, rec_idx] : m_ctx.text_index)
	{
		if (matched_keys.count(key))
			continue;

		m_ctx.esm_ref.select_record(rec_idx);
		m_ctx.esm_ref.set_value("TEXT");
		if (!m_ctx.esm_ref.get_value().exist)
			continue;

		const auto & old_text = m_ctx.esm_ref.get_value().text;
		insert_entry_base(key, old_text, old_text, rec_type_t::text, status_t::missing);
	}
}

void creator_base_t::make_rnam()
{
	m_ctx.reset_counters();
	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		if (m_ctx.esm.get_record().id != "FACT")
			continue;

		m_ctx.esm.set_key("NAME");
		m_ctx.esm.set_value("RNAM");
		if (!m_ctx.esm.get_key().exist)
			continue;

		while (m_ctx.esm.get_value().exist)
		{
			const auto key_text = m_ctx.esm.get_key().text + "^" + std::to_string(m_ctx.esm.get_value().counter);
			const auto & new_text = m_ctx.esm.get_value().text;

			auto search = m_ctx.rnam_index.find(key_text);
			if (search != m_ctx.rnam_index.end())
			{
				const auto & old_text = search->second;
				insert_entry_base(key_text, old_text, new_text, rec_type_t::rnam, status_t::translated);
			}
			else
			{
				insert_entry_base(key_text, "", new_text, rec_type_t::rnam, status_t::mismatch);
			}

			m_ctx.esm.set_next_value("RNAM");
		}
	}
}

void creator_base_t::make_indx()
{
	m_ctx.reset_counters();
	std::set<std::string> matched_keys;

	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		const auto & rec_id = m_ctx.esm.get_record().id;
		if (rec_id != "SKIL" && rec_id != "MGEF")
			continue;

		m_ctx.esm.set_key("INDX");
		m_ctx.esm.set_value("DESC");
		if (!m_ctx.esm.get_key().exist || !m_ctx.esm.get_value().exist)
			continue;

		const auto key_text = rec_id + "^" + domain_types_t::get_indx(m_ctx.esm.get_key().content);
		const auto & new_text = m_ctx.esm.get_value().text;

		auto search = m_ctx.indx_index.find(key_text);
		if (search != m_ctx.indx_index.end())
		{
			matched_keys.insert(key_text);
			m_ctx.esm_ref.select_record(search->second);
			m_ctx.esm_ref.set_key("INDX");
			m_ctx.esm_ref.set_value("DESC");
			const auto & old_text = m_ctx.esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, rec_type_t::indx, status_t::translated);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, rec_type_t::indx, status_t::mismatch);
		}
	}

	for (const auto & [key, rec_idx] : m_ctx.indx_index)
	{
		if (matched_keys.count(key))
			continue;

		m_ctx.esm_ref.select_record(rec_idx);
		m_ctx.esm_ref.set_value("DESC");
		if (!m_ctx.esm_ref.get_value().exist)
			continue;

		const auto & old_text = m_ctx.esm_ref.get_value().text;
		insert_entry_base(key, old_text, old_text, rec_type_t::indx, status_t::missing);
	}
}

void creator_base_t::make_info()
{
	std::string key_prefix;
	m_ctx.reset_counters();
	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		if (m_ctx.esm.get_record().id == "DIAL")
		{
			m_ctx.esm.set_key("DATA");
			m_ctx.esm.set_value("NAME");
			if (m_ctx.esm.get_key().exist && m_ctx.esm.get_value().exist)
			{
				const auto & native_name = m_ctx.esm.get_value().text;
				const auto dial_type = domain_types_t::get_dialog_type(m_ctx.esm.get_key().content);
				auto it_map = m_ctx.dial_native_to_foreign.find(native_name);
				if (it_map != m_ctx.dial_native_to_foreign.end())
					key_prefix = dial_type + "^" + it_map->second;
				else
					key_prefix = dial_type + "^" + native_name;
			}

			continue;
		}

		if (m_ctx.esm.get_record().id != "INFO")
			continue;

		m_ctx.esm.set_key("INAM");
		if (!m_ctx.esm.get_key().exist)
			continue;

		m_ctx.esm.set_value("NAME");
		if (!m_ctx.esm.get_value().exist)
			continue;

		const auto key_text = key_prefix + "^" + m_ctx.esm.get_key().text;
		const auto & new_text = m_ctx.esm.get_value().text;

		auto search = m_ctx.info_index.find(key_text);
		if (search != m_ctx.info_index.end())
		{
			m_ctx.esm_ref.select_record(search->second);
			m_ctx.esm_ref.set_key("INAM");
			m_ctx.esm_ref.set_value("NAME");
			const auto & old_text = m_ctx.esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, rec_type_t::info, status_t::translated);
		}
		else
		{
			insert_entry_base(key_text, "", new_text, rec_type_t::info, status_t::mismatch);
		}

		m_ctx.esm.select_record(i);
		m_ctx.esm.set_value("ONAM");
		if (!m_ctx.esm.get_value().exist || m_ctx.esm.get_value().text.empty())
			continue;

		const auto & speaker_id = m_ctx.esm.get_value().text;
		auto it_npc = m_ctx.npc_index.find(speaker_id);
		if (it_npc == m_ctx.npc_index.end())
			continue;

		m_ctx.esm_ref.select_record(it_npc->second);
		m_ctx.esm_ref.set_key("FNAM");
		m_ctx.esm_ref.set_value("FLAG");

		std::string speaker_name;
		if (m_ctx.esm_ref.get_key().exist)
			speaker_name = m_ctx.esm_ref.get_key().text;

		std::string gender;
		if (m_ctx.esm_ref.get_value().exist)
			gender =
			    ((domain_types_t::convert_string_byte_array_to_uint(m_ctx.esm_ref.get_value().content) & 0x0001) != 0) ? "F" : "M";

		auto * entry = m_ctx.dict.at(rec_type_t::info).find(key_text);
		if (!entry)
			continue;

		entry->speaker_name = speaker_name;
		entry->gender = gender;
	}
}

void creator_base_t::build_sctx_schd_index(std::unordered_map<std::string, size_t> & schd_index)
{
	for (size_t i = 0; i < m_ctx.esm_ref.get_records().size(); ++i)
	{
		m_ctx.esm_ref.select_record(i);
		if (m_ctx.esm_ref.get_record().id != "SCPT")
			continue;

		m_ctx.esm_ref.set_key("SCHD");
		if (!m_ctx.esm_ref.get_key().exist)
			continue;

		schd_index.insert({ m_ctx.esm_ref.get_key().text, i });
	}
}

void creator_base_t::match_sctx_messages(
    const std::string & script_name,
    const std::vector<std::string> & native_messages,
    const std::unordered_map<std::string, size_t> & schd_index)
{
	auto search = schd_index.find(script_name);
	if (search == schd_index.end())
	{
		app_logger_t::add_log("[warning] SCTX not found: \"" + script_name + "\"\r\n");
		for (const auto & msg : native_messages)
		{
			const auto key_text = script_name + "^" + msg;
			insert_entry_base(key_text, "", msg, rec_type_t::sctx, status_t::mismatch);
		}
		return;
	}

	m_ctx.esm_ref.select_record(search->second);
	m_ctx.esm_ref.set_key("SCHD");
	m_ctx.esm_ref.set_value("SCTX");
	if (!m_ctx.esm_ref.get_value().exist)
	{
		app_logger_t::add_log("[warning] SCTX not found: \"" + script_name + "\"\r\n");
		for (const auto & msg : native_messages)
		{
			const auto key_text = script_name + "^" + msg;
			insert_entry_base(key_text, "", msg, rec_type_t::sctx, status_t::mismatch);
		}
		return;
	}

	const auto foreign_messages = creator_helpers_t::make_script_messages(m_ctx.esm_ref.get_value().text);

	if (native_messages.size() != foreign_messages.size())
	{
		app_logger_t::add_log(
		    "[warning] SCTX line count mismatch: \"" + script_name + "\" (native=" +
		    std::to_string(native_messages.size()) + ", foreign=" + std::to_string(foreign_messages.size()) + ")\r\n");

		for (const auto & msg : foreign_messages)
		{
			const auto key_text = script_name + "^" + msg;
			insert_entry_base(key_text, msg, msg, rec_type_t::sctx, status_t::mismatch);
		}
		return;
	}

	for (size_t k = 0; k < native_messages.size(); ++k)
	{
		const auto key_text = script_name + "^" + foreign_messages[k];
		const auto & old_text = foreign_messages[k];
		const auto & new_text = native_messages[k];
		insert_entry_base(key_text, old_text, new_text, rec_type_t::sctx, status_t::translated);
	}
}

void creator_base_t::make_sctx()
{
	m_ctx.reset_counters();

	std::unordered_map<std::string, size_t> schd_index;
	build_sctx_schd_index(schd_index);

	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		if (m_ctx.esm.get_record().id != "SCPT")
			continue;

		m_ctx.esm.set_key("SCHD");
		m_ctx.esm.set_value("SCTX");
		if (!m_ctx.esm.get_key().exist || !m_ctx.esm.get_value().exist)
			continue;

		const auto & script_name = m_ctx.esm.get_key().text;
		const auto native_messages = creator_helpers_t::make_script_messages(m_ctx.esm.get_value().text);
		match_sctx_messages(script_name, native_messages, schd_index);
	}
}

void creator_base_t::match_bnam_native_infos(
    const std::string & info_key,
    const std::vector<std::string> & native_messages)
{
	auto search = m_ctx.info_index.find(info_key);
	if (search == m_ctx.info_index.end())
	{
		for (const auto & msg : native_messages)
		{
			const auto key_text = info_key + "^" + msg;
			insert_entry_base(key_text, "", msg, rec_type_t::bnam, status_t::mismatch);
		}
		return;
	}

	m_ctx.esm_ref.select_record(search->second);
	m_ctx.esm_ref.set_value("BNAM");
	if (!m_ctx.esm_ref.get_value().exist || m_ctx.esm_ref.get_value().text.empty())
	{
		for (const auto & msg : native_messages)
		{
			const auto key_text = info_key + "^" + msg;
			insert_entry_base(key_text, "", msg, rec_type_t::bnam, status_t::mismatch);
		}
		return;
	}

	const auto foreign_messages = creator_helpers_t::make_script_messages(m_ctx.esm_ref.get_value().text);

	if (native_messages.size() != foreign_messages.size())
	{
		app_logger_t::add_log("[warning] BNAM line count mismatch: \"" + info_key + "\"\r\n");
		for (const auto & msg : foreign_messages)
		{
			const auto key_text = info_key + "^" + msg;
			insert_entry_base(key_text, msg, msg, rec_type_t::bnam, status_t::mismatch);
		}
		return;
	}

	for (size_t k = 0; k < native_messages.size(); ++k)
	{
		const auto key_text = info_key + "^" + foreign_messages[k];
		const auto & old_text = foreign_messages[k];
		const auto & new_text = native_messages[k];
		insert_entry_base(key_text, old_text, new_text, rec_type_t::bnam, status_t::translated);
	}
}

void creator_base_t::collect_bnam_missing_topics(const std::set<std::string> & matched_foreign_topics)
{
	std::string foreign_dial_type;
	std::string foreign_dial_name;

	for (size_t i = 0; i < m_ctx.esm_ref.get_records().size(); ++i)
	{
		m_ctx.esm_ref.select_record(i);
		const auto & rec_id = m_ctx.esm_ref.get_record().id;

		if (rec_id == "DIAL")
		{
			m_ctx.esm_ref.set_key("DATA");
			m_ctx.esm_ref.set_value("NAME");
			if (m_ctx.esm_ref.get_key().exist && m_ctx.esm_ref.get_value().exist)
			{
				foreign_dial_type = domain_types_t::get_dialog_type(m_ctx.esm_ref.get_key().content);
				foreign_dial_name = m_ctx.esm_ref.get_value().text;
			}
			continue;
		}

		if (rec_id != "INFO")
			continue;

		if (matched_foreign_topics.count(foreign_dial_name))
			continue;

		m_ctx.esm_ref.set_key("INAM");
		if (!m_ctx.esm_ref.get_key().exist)
			continue;

		const auto & inam = m_ctx.esm_ref.get_key().text;

		m_ctx.esm_ref.set_value("BNAM");
		if (!m_ctx.esm_ref.get_value().exist || m_ctx.esm_ref.get_value().text.empty())
			continue;

		const auto foreign_messages = creator_helpers_t::make_script_messages(m_ctx.esm_ref.get_value().text);
		for (const auto & msg : foreign_messages)
		{
			const auto key_text = foreign_dial_type + "^" + foreign_dial_name + "^" + inam + "^" + msg;
			insert_entry_base(key_text, msg, msg, rec_type_t::bnam, status_t::missing);
		}
	}
}

void creator_base_t::make_bnam()
{
	std::string dial_type;
	std::string dial_name;
	std::string info_inam;
	m_ctx.reset_counters();

	std::set<std::string> matched_foreign_topics;
	for (const auto & [native, foreign] : m_ctx.dial_native_to_foreign)
		matched_foreign_topics.insert(foreign);

	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		const auto & rec_id = m_ctx.esm.get_record().id;

		if (rec_id == "DIAL")
		{
			m_ctx.esm.set_key("DATA");
			m_ctx.esm.set_value("NAME");
			if (m_ctx.esm.get_key().exist && m_ctx.esm.get_value().exist)
			{
				dial_type = domain_types_t::get_dialog_type(m_ctx.esm.get_key().content);
				const auto & native_name = m_ctx.esm.get_value().text;
				auto it_map = m_ctx.dial_native_to_foreign.find(native_name);
				if (it_map != m_ctx.dial_native_to_foreign.end())
					dial_name = it_map->second;
				else
					dial_name.clear();
			}
			continue;
		}

		if (rec_id != "INFO")
			continue;

		if (dial_name.empty())
			continue;

		m_ctx.esm.set_key("INAM");
		if (!m_ctx.esm.get_key().exist)
			continue;

		info_inam = m_ctx.esm.get_key().text;

		m_ctx.esm.set_value("BNAM");
		if (!m_ctx.esm.get_value().exist || m_ctx.esm.get_value().text.empty())
			continue;

		const auto native_messages = creator_helpers_t::make_script_messages(m_ctx.esm.get_value().text);
		const auto info_key = dial_type + "^" + dial_name + "^" + info_inam;
		match_bnam_native_infos(info_key, native_messages);
	}

	collect_bnam_missing_topics(matched_foreign_topics);
}

void creator_base_t::make_dial()
{
	m_ctx.reset_counters();
	auto status_fn = [this](const std::string & old_text, const std::string & new_text)
	{ return creator_helpers_t::determine_status(m_ctx, old_text, new_text); };
	dial_matcher_t dial_matcher(m_ctx.esm, m_ctx.esm_ref, m_ctx.translation_engine, m_ctx.dict, status_fn);
	dial_matcher.match_topics();
	m_ctx.dial_native_to_foreign = dial_matcher.get_native_to_foreign();
}

void creator_base_t::make_cell()
{
	auto status_fn = [this](const std::string & old_text, const std::string & new_text)
	{ return creator_helpers_t::determine_status(m_ctx, old_text, new_text); };
	cell_matcher_t cell_matcher(m_ctx.esm, m_ctx.esm_ref, m_ctx.translation_engine, m_ctx.dict, status_fn);
	cell_matcher.match_exterior_cells();
	cell_matcher.match_interior_cells();
	cell_matcher.match_default_cell_name();
	cell_matcher.match_region_names();
}

void creator_base_t::insert_entry_base(
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    rec_type_t type,
    status_t status)
{
	m_ctx.counter_all++;

	const bool is_text_keyed =
	    (type == rec_type_t::cell || type == rec_type_t::dial || type == rec_type_t::sctx ||
	     type == rec_type_t::bnam);

	auto * existing = m_ctx.dict.at(type).find(key_text);
	if (existing && is_text_keyed)
	{
		if (existing->old_text == old_text && existing->new_text == new_text)
			return;
	}

	const bool is_problem =
	    (status == status_t::missing || status == status_t::mismatch || status == status_t::duplicate);

	const bool is_status = is_problem || status == status_t::heuristic;

	record_entry_t entry;
	entry.key_text = key_text;
	entry.old_text = old_text;
	entry.new_text = new_text;
	entry.status = is_status ? status : creator_helpers_t::determine_status(m_ctx, old_text, new_text);

	if (m_ctx.dict.at(type).insert(entry))
	{
		m_ctx.counter_created++;
		return;
	}

	auto * dup = m_ctx.dict.at(type).find(key_text);
	if (dup)
	{
		if (dup->old_text == old_text && dup->new_text == new_text)
			return;

		dup->status = status_t::duplicate;
		if (dup->details.empty())
			dup->details = new_text;
		else
			dup->details += "|" + new_text;

		m_ctx.counter_doubled++;
	}
}
