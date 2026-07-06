#include "eet_converter.hpp"
#include "../utility/app_logger.hpp"

eet_converter_t::eet_converter_t(const std::vector<eet_reader_t::eet_entry_t> & entries)
{
	m_dict = domain_types_t::initialize_dict();

	for (const auto & entry : entries)
	{
		const auto yampt_type = map_type(entry.rec_type, entry.sub_type);
		if (yampt_type == rec_type_t::unknown)
		{
			++m_skipped_count;
			continue;
		}

		const auto status = map_status(entry.status_byte);
		const auto & key_text = build_key_text(entry, yampt_type);

		record_entry_t record;
		record.key_text = key_text;
		record.old_text = entry.orig;
		record.new_text = entry.trans;
		record.status = status;

		m_dict.at(yampt_type).insert(record);
		++m_converted_count;
	}

	app_logger_t::add_log(
	    "[info] EET conversion: converted=" + std::to_string(m_converted_count) +
	    ", skipped=" + std::to_string(m_skipped_count) + "\r\n");
}

rec_type_t eet_converter_t::map_type(const std::string & rec_type, const std::string & sub_type) const
{
	if (sub_type == "FNAM")
	{
		if (rec_type == "NPC_" || rec_type == "SPEL" || rec_type == "ARMO" || rec_type == "BOOK" ||
		    rec_type == "CONT" || rec_type == "MISC" || rec_type == "CLOT" || rec_type == "CREA" ||
		    rec_type == "ALCH" || rec_type == "DOOR" || rec_type == "ACTI" || rec_type == "LIGH" ||
		    rec_type == "INGR" || rec_type == "CLAS" || rec_type == "FACT" || rec_type == "APPA" || rec_type == "REPA")
		{
			return rec_type_t::fnam;
		}

		if (rec_type == "CELL" || rec_type == "REGN")
			return rec_type_t::cell;
	}

	if (sub_type == "NAME")
	{
		if (rec_type == "CELL" || rec_type == "REGN")
			return rec_type_t::cell;

		if (rec_type == "PGRD")
			return rec_type_t::cell;

		if (rec_type == "DIAL")
			return rec_type_t::dial;
	}

	if (sub_type == "DNAM")
	{
		if (rec_type == "CELL")
			return rec_type_t::cell;

		if (rec_type == "NPC_")
			return rec_type_t::fnam;
	}

	if (sub_type == "CNDT" && rec_type == "NPC_")
		return rec_type_t::fnam;

	if (sub_type == "TEXT" && rec_type == "BOOK")
		return rec_type_t::text;

	if (sub_type == "SCTX" && rec_type == "SCPT")
		return rec_type_t::sctx;

	if (rec_type == "SCPT")
	{
		if (sub_type == "MSGB" || sub_type == "CELL" || sub_type == "SAY_" || sub_type == "DIAL")
			return rec_type_t::bnam;
	}

	if (sub_type == "DESC")
	{
		if (rec_type == "MGEF" || rec_type == "CLAS")
			return rec_type_t::desc;
	}

	if (sub_type == "RNAM" && rec_type == "FACT")
		return rec_type_t::rnam;

	if (sub_type == "STRV" && rec_type == "GMST")
		return rec_type_t::gmst;

	return rec_type_t::unknown;
}

status_t eet_converter_t::map_status(uint8_t status_byte) const
{
	if (status_byte == 0x63)
		return status_t::translated;

	return status_t::untranslated;
}

std::string eet_converter_t::build_key_text(const eet_reader_t::eet_entry_t & entry, rec_type_t yampt_type)
    const
{
	if (yampt_type == rec_type_t::cell || yampt_type == rec_type_t::dial ||
	    yampt_type == rec_type_t::sctx || yampt_type == rec_type_t::bnam)
	{
		return entry.orig;
	}

	return entry.key_text;
}
