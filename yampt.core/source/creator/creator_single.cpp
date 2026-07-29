#include "creator_single.hpp"
#include "creator_helpers.hpp"

creator_single_t::creator_single_t(creator_context_t & context)
    : m_ctx(context)
    , m_with_base(context.base_dict != nullptr)
{}

void creator_single_t::run()
{
	creator_helpers::build_npc_index(m_ctx);
	creator_helpers::build_text_match_index(m_ctx);

	make_gmst();
	make_fnam();
	make_desc();
	make_text();
	make_rnam();
	make_indx();
	make_info();
	make_sctx();
	make_script();
	make_bnam();
	make_dial();
	make_cell();
}

void creator_single_t::make_gmst()
{
	m_ctx.reset_counters();
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
		const auto & text = m_ctx.esm.get_value().text;

		if (m_with_base)
			creator_helpers::insert_entry_single_with_base(m_ctx, key_text, text, text, rec_type_t::gmst);
		else
			creator_helpers::insert_entry_single(m_ctx, key_text, text, text, rec_type_t::gmst);
	}
}

void creator_single_t::make_fnam()
{
	m_ctx.reset_counters();
	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		if (!domain_types::is_fnam(m_ctx.esm.get_record().id))
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
		const auto & text = m_ctx.esm.get_value().text;

		if (m_with_base)
			creator_helpers::insert_entry_single_with_base(m_ctx, key_text, text, text, rec_type_t::fnam);
		else
			creator_helpers::insert_entry_single(m_ctx, key_text, text, text, rec_type_t::fnam);
	}
}

void creator_single_t::make_desc()
{
	m_ctx.reset_counters();
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
		const auto & text = m_ctx.esm.get_value().text;

		if (m_with_base)
			creator_helpers::insert_entry_single_with_base(m_ctx, key_text, text, text, rec_type_t::desc);
		else
			creator_helpers::insert_entry_single(m_ctx, key_text, text, text, rec_type_t::desc);
	}
}

void creator_single_t::make_text()
{
	m_ctx.reset_counters();
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
		const auto & text = m_ctx.esm.get_value().text;

		if (m_with_base)
			creator_helpers::insert_entry_single_with_base(m_ctx, key_text, text, text, rec_type_t::text);
		else
			creator_helpers::insert_entry_single(m_ctx, key_text, text, text, rec_type_t::text);
	}
}

void creator_single_t::make_rnam()
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
			const auto & text = m_ctx.esm.get_value().text;

			if (m_with_base)
				creator_helpers::insert_entry_single_with_base(m_ctx, key_text, text, text, rec_type_t::rnam);
			else
				creator_helpers::insert_entry_single(m_ctx, key_text, text, text, rec_type_t::rnam);

			m_ctx.esm.set_next_value("RNAM");
		}
	}
}

void creator_single_t::make_indx()
{
	m_ctx.reset_counters();
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

		const auto key_text = rec_id + "^" + domain_types::get_indx(m_ctx.esm.get_key().content);
		const auto & text = m_ctx.esm.get_value().text;

		if (m_with_base)
			creator_helpers::insert_entry_single_with_base(m_ctx, key_text, text, text, rec_type_t::indx);
		else
			creator_helpers::insert_entry_single(m_ctx, key_text, text, text, rec_type_t::indx);
	}
}

void creator_single_t::make_info()
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
				key_prefix =
				    domain_types::get_dialog_type(m_ctx.esm.get_key().content) + "^" + m_ctx.esm.get_value().text;

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
		const auto & text = m_ctx.esm.get_value().text;

		if (m_with_base)
			creator_helpers::insert_entry_single_with_base(m_ctx, key_text, text, text, rec_type_t::info);
		else
			creator_helpers::insert_entry_single(m_ctx, key_text, text, text, rec_type_t::info);

		creator_helpers::enrich_info_speaker(m_ctx, key_text, i);
	}
}

void creator_single_t::make_sctx()
{
	m_ctx.reset_counters();
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
		const auto & messages = creator_helpers::make_script_messages(m_ctx.esm.get_value().text);

		for (size_t k = 0; k < messages.size(); ++k)
		{
			const auto key_text = script_name + "^" + messages[k];
			const auto & old_text = messages[k];

			if (m_with_base)
				creator_helpers::insert_entry_single_with_base(m_ctx, key_text, old_text, old_text, rec_type_t::sctx);
			else
				creator_helpers::insert_entry_single(m_ctx, key_text, old_text, old_text, rec_type_t::sctx);
		}
	}
}

void creator_single_t::make_script()
{
	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		if (m_ctx.esm.get_record().id != "SCPT")
			continue;

		m_ctx.esm.set_key("SCHD");
		m_ctx.esm.set_value("SCTX");
		if (!m_ctx.esm.get_key().exist || !m_ctx.esm.get_value().exist)
			continue;

		record_entry_t entry;
		entry.key_text = m_ctx.esm.get_key().text;
		entry.old_text = m_ctx.esm.get_value().text;
		entry.status = status_t::translated;
		m_ctx.dict.at(rec_type_t::script).insert(entry);
	}
}

void creator_single_t::make_bnam()
{
	std::string dial_type;
	std::string dial_name;
	std::string info_inam;
	m_ctx.reset_counters();
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
				dial_type = domain_types::get_dialog_type(m_ctx.esm.get_key().content);
				dial_name = m_ctx.esm.get_value().text;
			}
			continue;
		}

		if (rec_id != "INFO")
			continue;

		m_ctx.esm.set_key("INAM");
		if (!m_ctx.esm.get_key().exist)
			continue;

		info_inam = m_ctx.esm.get_key().text;

		m_ctx.esm.set_value("BNAM");
		if (!m_ctx.esm.get_value().exist || m_ctx.esm.get_value().text.empty())
			continue;

		const auto & messages = creator_helpers::make_script_messages(m_ctx.esm.get_value().text);
		for (size_t k = 0; k < messages.size(); ++k)
		{
			const auto key_text = dial_type + "^" + dial_name + "^" + info_inam + "^" + messages[k];
			const auto & old_text = messages[k];

			if (m_with_base)
				creator_helpers::insert_entry_single_with_base(m_ctx, key_text, old_text, old_text, rec_type_t::bnam);
			else
				creator_helpers::insert_entry_single(m_ctx, key_text, old_text, old_text, rec_type_t::bnam);
		}
	}
}

void creator_single_t::make_dial()
{
	m_ctx.reset_counters();
	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		if (m_ctx.esm.get_record().id != "DIAL")
			continue;

		m_ctx.esm.set_key("DATA");
		m_ctx.esm.set_value("NAME");
		if (domain_types::get_dialog_type(m_ctx.esm.get_key().content) != "T")
			continue;

		if (!m_ctx.esm.get_value().exist)
			continue;

		const auto & text = m_ctx.esm.get_value().text;

		if (m_with_base)
			creator_helpers::insert_entry_single_with_base(m_ctx, text, text, text, rec_type_t::dial);
		else
			creator_helpers::insert_entry_single(m_ctx, text, text, text, rec_type_t::dial);
	}
}

void creator_single_t::make_cell()
{
	m_ctx.reset_counters();
	for (size_t i = 0; i < m_ctx.esm.get_records().size(); ++i)
	{
		m_ctx.esm.select_record(i);
		if (m_ctx.esm.get_record().id != "CELL")
			continue;

		m_ctx.esm.set_value("NAME");
		if (!m_ctx.esm.get_value().exist || m_ctx.esm.get_value().text.empty())
			continue;

		const auto & text = m_ctx.esm.get_value().text;

		if (m_with_base)
			creator_helpers::insert_entry_single_with_base(m_ctx, text, text, text, rec_type_t::cell);
		else
			creator_helpers::insert_entry_single(m_ctx, text, text, text, rec_type_t::cell);
	}
}
