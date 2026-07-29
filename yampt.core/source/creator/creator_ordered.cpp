#include "creator_ordered.hpp"
#include "../utility/app_logger.hpp"
#include "creator_helpers.hpp"

creator_ordered_t::creator_ordered_t(creator_context_t & context)
    : m_ctx(context)
{}

void creator_ordered_t::run()
{
	creator_helpers::load_english_dict(m_ctx);

	creator_helpers::build_npc_index(m_ctx);
	build_dial_map();

	std::string dial_type;
	std::string dial_foreign_name;

	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		m_ctx.esm_ref.select_record(i);
		const auto & rec_id = m_ctx.esm.get_record().id;

		if (rec_id == "GMST")
			process_gmst(i);
		else if (rec_id == "DIAL")
			process_dial(i, dial_type, dial_foreign_name);
		else if (rec_id == "INFO")
			process_info(i, dial_type, dial_foreign_name);
		else if (rec_id == "SCPT")
			process_sctx(i);
		else if (rec_id == "CELL")
			process_cell(i);
		else if (rec_id == "BOOK")
			process_text(i);
		else if (rec_id == "FACT")
			process_rnam(i);
		else if (rec_id == "BSGN" || rec_id == "CLAS" || rec_id == "RACE")
			process_desc(i);
		else if (rec_id == "SKIL" || rec_id == "MGEF")
			process_indx(i);
	}

	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		m_ctx.esm_ref.select_record(i);

		if (!domain_types::is_fnam(m_ctx.esm.get_record().id))
			continue;

		process_fnam(i);
	}

	process_cell_default();
	process_cell_region();
}

void creator_ordered_t::build_dial_map()
{
	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		if (m_ctx.esm.get_record().id != "DIAL")
			continue;

		m_ctx.esm.set_key("DATA");
		if (!m_ctx.esm.get_key().exist)
			continue;

		if (domain_types::get_dialog_type(m_ctx.esm.get_key().content) != "T")
			continue;

		m_ctx.esm.set_value("NAME");
		m_ctx.esm_ref.select_record(i);
		m_ctx.esm_ref.set_value("NAME");

		if (m_ctx.esm.get_value().exist && m_ctx.esm_ref.get_value().exist)
			m_ctx.dial_native_to_foreign[m_ctx.esm.get_value().text] = m_ctx.esm_ref.get_value().text;
	}
}

void creator_ordered_t::process_gmst(size_t i)
{
	m_ctx.esm.set_key("NAME");
	m_ctx.esm.set_value("STRV");
	if (!m_ctx.esm.get_key().exist || !m_ctx.esm.get_value().exist)
		return;

	if (m_ctx.esm.get_key().text.substr(0, 1) != "s")
		return;

	const auto & key_text = m_ctx.esm.get_key().text;
	const auto & new_text = m_ctx.esm.get_value().text;

	m_ctx.esm_ref.set_key("NAME");
	m_ctx.esm_ref.set_value("STRV");
	if (!m_ctx.esm_ref.get_value().exist)
		return;

	const auto & old_text = m_ctx.esm_ref.get_value().text;
	insert_entry_base(key_text, old_text, new_text, rec_type_t::gmst, status_t::translated);
}

void creator_ordered_t::process_fnam(size_t i)
{
	m_ctx.esm.set_key("NAME");
	m_ctx.esm.set_value("FNAM");
	if (!m_ctx.esm.get_key().exist || !m_ctx.esm.get_value().exist)
		return;

	if (m_ctx.esm.get_key().text == "player")
		return;

	if (m_ctx.esm.get_value().text.empty())
		return;

	const auto key_text = m_ctx.esm.get_record().id + "^" + m_ctx.esm.get_key().text;
	const auto & new_text = m_ctx.esm.get_value().text;

	m_ctx.esm_ref.set_key("NAME");
	m_ctx.esm_ref.set_value("FNAM");
	if (!m_ctx.esm_ref.get_value().exist)
		return;

	const auto & old_text = m_ctx.esm_ref.get_value().text;
	insert_entry_base(key_text, old_text, new_text, rec_type_t::fnam, status_t::translated);
}

void creator_ordered_t::process_desc(size_t i)
{
	m_ctx.esm.set_key("NAME");
	m_ctx.esm.set_value("DESC");
	if (!m_ctx.esm.get_key().exist || !m_ctx.esm.get_value().exist)
		return;

	const auto key_text = m_ctx.esm.get_record().id + "^" + m_ctx.esm.get_key().text;
	const auto & new_text = m_ctx.esm.get_value().text;

	m_ctx.esm_ref.set_key("NAME");
	m_ctx.esm_ref.set_value("DESC");
	if (!m_ctx.esm_ref.get_value().exist)
		return;

	const auto & old_text = m_ctx.esm_ref.get_value().text;
	insert_entry_base(key_text, old_text, new_text, rec_type_t::desc, status_t::translated);
}

void creator_ordered_t::process_text(size_t i)
{
	m_ctx.esm.set_key("NAME");
	m_ctx.esm.set_value("TEXT");
	if (!m_ctx.esm.get_key().exist || !m_ctx.esm.get_value().exist)
		return;

	const auto & key_text = m_ctx.esm.get_key().text;
	const auto & new_text = m_ctx.esm.get_value().text;

	m_ctx.esm_ref.set_key("NAME");
	m_ctx.esm_ref.set_value("TEXT");
	if (!m_ctx.esm_ref.get_value().exist)
		return;

	const auto & old_text = m_ctx.esm_ref.get_value().text;
	insert_entry_base(key_text, old_text, new_text, rec_type_t::text, status_t::translated);
}

void creator_ordered_t::process_rnam(size_t i)
{
	m_ctx.esm.set_key("NAME");
	m_ctx.esm.set_value("RNAM");
	if (!m_ctx.esm.get_key().exist)
		return;

	m_ctx.esm_ref.set_key("NAME");
	m_ctx.esm_ref.set_value("RNAM");

	while (m_ctx.esm.get_value().exist)
	{
		const auto key_text = m_ctx.esm.get_key().text + "^" + std::to_string(m_ctx.esm.get_value().counter);
		const auto & new_text = m_ctx.esm.get_value().text;

		if (m_ctx.esm_ref.get_value().exist)
		{
			const auto & old_text = m_ctx.esm_ref.get_value().text;
			insert_entry_base(key_text, old_text, new_text, rec_type_t::rnam, status_t::translated);
			m_ctx.esm_ref.set_next_value("RNAM");
		}
		else
		{
			app_logger_t::add_log("[warning] RNAM count mismatch: \"" + m_ctx.esm.get_key().text + "\"\r\n");
			insert_entry_base(key_text, "", new_text, rec_type_t::rnam, status_t::mismatch);
		}

		m_ctx.esm.set_next_value("RNAM");
	}
}

void creator_ordered_t::process_indx(size_t i)
{
	m_ctx.esm.set_key("INDX");
	m_ctx.esm.set_value("DESC");
	if (!m_ctx.esm.get_key().exist || !m_ctx.esm.get_value().exist)
		return;

	const auto key_text = m_ctx.esm.get_record().id + "^" + domain_types::get_indx(m_ctx.esm.get_key().content);
	const auto & new_text = m_ctx.esm.get_value().text;

	m_ctx.esm_ref.set_key("INDX");
	m_ctx.esm_ref.set_value("DESC");
	if (!m_ctx.esm_ref.get_value().exist)
		return;

	const auto & old_text = m_ctx.esm_ref.get_value().text;
	insert_entry_base(key_text, old_text, new_text, rec_type_t::indx, status_t::translated);
}

void creator_ordered_t::process_cell(size_t i)
{
	m_ctx.esm.set_value("NAME");
	if (!m_ctx.esm.get_value().exist || m_ctx.esm.get_value().text.empty())
		return;

	const auto & new_text = m_ctx.esm.get_value().text;

	m_ctx.esm_ref.set_value("NAME");
	if (!m_ctx.esm_ref.get_value().exist || m_ctx.esm_ref.get_value().text.empty())
		return;

	const auto & old_text = m_ctx.esm_ref.get_value().text;
	insert_entry_base(old_text, old_text, new_text, rec_type_t::cell, status_t::translated);
}

void creator_ordered_t::process_dial(size_t i, std::string & dial_type, std::string & dial_foreign_name)
{
	m_ctx.esm.set_key("DATA");
	if (!m_ctx.esm.get_key().exist)
		return;

	dial_type = domain_types::get_dialog_type(m_ctx.esm.get_key().content);

	m_ctx.esm.set_value("NAME");
	if (!m_ctx.esm.get_value().exist)
		return;

	const auto & native_name = m_ctx.esm.get_value().text;

	m_ctx.esm_ref.set_value("NAME");
	if (!m_ctx.esm_ref.get_value().exist)
		return;

	dial_foreign_name = m_ctx.esm_ref.get_value().text;

	if (dial_type != "T")
		return;

	insert_entry_base(dial_foreign_name, dial_foreign_name, native_name, rec_type_t::dial, status_t::translated);
}

void creator_ordered_t::attach_speaker_metadata(const std::string & key_text, size_t record_index)
{
	m_ctx.esm.select_record(record_index);
	m_ctx.esm.set_value("ONAM");
	if (!m_ctx.esm.get_value().exist || m_ctx.esm.get_value().text.empty())
		return;

	const auto & speaker_id = m_ctx.esm.get_value().text;
	const auto npc_search = m_ctx.npc_index.find(speaker_id);
	if (npc_search == m_ctx.npc_index.end())
		return;

	m_ctx.esm_ref.select_record(npc_search->second);
	m_ctx.esm_ref.set_key("FNAM");
	m_ctx.esm_ref.set_value("FLAG");

	std::string speaker_name;
	if (m_ctx.esm_ref.get_key().exist)
		speaker_name = m_ctx.esm_ref.get_key().text;

	std::string gender;
	if (m_ctx.esm_ref.get_value().exist)
		gender = ((domain_types::convert_string_byte_array_to_uint(m_ctx.esm_ref.get_value().content) & 0x0001) != 0)
		             ? "F"
		             : "M";

	auto * entry = m_ctx.dict.at(rec_type_t::info).find(key_text);
	if (!entry)
		return;

	entry->speaker_name = speaker_name;
	entry->gender = gender;
}

void creator_ordered_t::process_info(size_t i, const std::string & dial_type, const std::string & dial_foreign_name)
{
	m_ctx.esm.set_key("INAM");
	if (!m_ctx.esm.get_key().exist)
		return;

	const auto inam = m_ctx.esm.get_key().text;

	m_ctx.esm.set_value("NAME");
	if (!m_ctx.esm.get_value().exist)
		return;

	const auto & new_text = m_ctx.esm.get_value().text;

	m_ctx.esm_ref.set_value("NAME");
	if (!m_ctx.esm_ref.get_value().exist)
		return;

	const auto & old_text = m_ctx.esm_ref.get_value().text;

	const auto key_text = dial_type + "^" + dial_foreign_name + "^" + inam;
	insert_entry_base(key_text, old_text, new_text, rec_type_t::info, status_t::translated);

	process_bnam(i, dial_type, dial_foreign_name, inam);
	attach_speaker_metadata(key_text, i);
}

void creator_ordered_t::process_sctx(size_t i)
{
	m_ctx.esm.set_key("SCHD");
	m_ctx.esm.set_value("SCTX");
	if (!m_ctx.esm.get_key().exist || !m_ctx.esm.get_value().exist)
		return;

	const auto & script_name = m_ctx.esm.get_key().text;
	const auto native_messages = creator_helpers::make_script_messages(m_ctx.esm.get_value().text);

	m_ctx.esm_ref.set_key("SCHD");
	m_ctx.esm_ref.set_value("SCTX");
	if (!m_ctx.esm_ref.get_value().exist)
	{
		app_logger_t::add_log("[warning] SCTX not found: \"" + script_name + "\"\r\n");
		for (const auto & msg : native_messages)
			insert_entry_base(script_name + "^" + msg, "", msg, rec_type_t::sctx, status_t::mismatch);
		return;
	}

	record_entry_t script_entry;
	script_entry.key_text = script_name;
	script_entry.old_text = m_ctx.esm_ref.get_value().text;
	script_entry.status = status_t::translated;
	m_ctx.dict.at(rec_type_t::script).insert(script_entry);

	const auto foreign_messages = creator_helpers::make_script_messages(m_ctx.esm_ref.get_value().text);

	if (native_messages.size() != foreign_messages.size())
	{
		app_logger_t::add_log("[warning] SCTX line count mismatch: \"" + script_name + "\"\r\n");
		for (const auto & msg : foreign_messages)
			insert_entry_base(script_name + "^" + msg, msg, msg, rec_type_t::sctx, status_t::mismatch);
		return;
	}

	for (size_t k = 0; k < native_messages.size(); ++k)
		insert_entry_base(
		    script_name + "^" + foreign_messages[k],
		    foreign_messages[k],
		    native_messages[k],
		    rec_type_t::sctx,
		    status_t::translated);
}

void creator_ordered_t::process_bnam(
    size_t i,
    const std::string & dial_type,
    const std::string & dial_foreign_name,
    const std::string & info_inam)
{
	m_ctx.esm.select_record(i);
	m_ctx.esm_ref.select_record(i);

	m_ctx.esm.set_value("BNAM");
	if (!m_ctx.esm.get_value().exist || m_ctx.esm.get_value().text.empty())
		return;

	const auto native_messages = creator_helpers::make_script_messages(m_ctx.esm.get_value().text);

	m_ctx.esm_ref.set_value("BNAM");
	if (!m_ctx.esm_ref.get_value().exist || m_ctx.esm_ref.get_value().text.empty())
	{
		const auto info_key = dial_type + "^" + dial_foreign_name + "^" + info_inam;
		app_logger_t::add_log("[warning] BNAM not found: \"" + info_key + "\"\r\n");
		for (const auto & msg : native_messages)
		{
			const auto key_text = dial_type + "^" + dial_foreign_name + "^" + info_inam + "^" + msg;
			insert_entry_base(key_text, "", msg, rec_type_t::bnam, status_t::mismatch);
		}
		return;
	}

	const auto foreign_messages = creator_helpers::make_script_messages(m_ctx.esm_ref.get_value().text);

	if (native_messages.size() != foreign_messages.size())
	{
		const auto info_key = dial_type + "^" + dial_foreign_name + "^" + info_inam;
		app_logger_t::add_log("[warning] BNAM line count mismatch: \"" + info_key + "\"\r\n");
		for (const auto & msg : foreign_messages)
		{
			const auto key_text = dial_type + "^" + dial_foreign_name + "^" + info_inam + "^" + msg;
			insert_entry_base(key_text, msg, msg, rec_type_t::bnam, status_t::mismatch);
		}
		return;
	}

	for (size_t k = 0; k < native_messages.size(); ++k)
	{
		const auto key_text = dial_type + "^" + dial_foreign_name + "^" + info_inam + "^" + foreign_messages[k];
		const auto & old_text = foreign_messages[k];
		const auto & new_text = native_messages[k];
		insert_entry_base(key_text, old_text, new_text, rec_type_t::bnam, status_t::translated);
	}
}

void creator_ordered_t::process_cell_default()
{
	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		if (m_ctx.esm.get_record().id != "GMST")
			continue;

		m_ctx.esm.set_key("NAME");
		if (!m_ctx.esm.get_key().exist)
			continue;

		if (m_ctx.esm.get_key().text != "sDefaultCellname")
			continue;

		m_ctx.esm.set_value("STRV");
		if (!m_ctx.esm.get_value().exist)
			break;

		const auto & new_text = m_ctx.esm.get_value().text;

		m_ctx.esm_ref.select_record(i);
		m_ctx.esm_ref.set_key("NAME");
		m_ctx.esm_ref.set_value("STRV");
		if (!m_ctx.esm_ref.get_value().exist)
			break;

		const auto & old_text = m_ctx.esm_ref.get_value().text;
		insert_entry_base(old_text, old_text, new_text, rec_type_t::cell, status_t::translated);
		break;
	}
}

void creator_ordered_t::process_cell_region()
{
	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		if (m_ctx.esm.get_record().id != "REGN")
			continue;

		m_ctx.esm.set_value("FNAM");
		if (!m_ctx.esm.get_value().exist || m_ctx.esm.get_value().text.empty())
			continue;

		const auto & new_text = m_ctx.esm.get_value().text;

		m_ctx.esm_ref.select_record(i);
		m_ctx.esm_ref.set_value("FNAM");
		if (!m_ctx.esm_ref.get_value().exist || m_ctx.esm_ref.get_value().text.empty())
			continue;

		const auto & old_text = m_ctx.esm_ref.get_value().text;
		insert_entry_base(old_text, old_text, new_text, rec_type_t::cell, status_t::translated);
	}
}

void creator_ordered_t::insert_entry_base(
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text,
    rec_type_t type,
    status_t status)
{
	m_ctx.counter_all++;

	const bool is_text_keyed =
	    (type == rec_type_t::cell || type == rec_type_t::dial || type == rec_type_t::sctx || type == rec_type_t::bnam);

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
	entry.status = is_status ? status : creator_helpers::determine_status(m_ctx, old_text, new_text);

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
