#include "esm_converter.hpp"
#include "script_parser.hpp"

esm_converter_t::esm_converter_t(
    const std::string & path,
    const dict_merger_t & merger,
    const bool add_hyperlinks,
    const std::string & file_suffix,
    const tools_t::encoding_t encoding,
    const bool create_header)
    : esm(path)
    , merger(merger)
    , add_hyperlinks(add_hyperlinks)
    , file_suffix(file_suffix)
    , create_header(create_header)
{
	if (encoding == tools_t::encoding_t::windows_1250)
	{
		esm_encoding = detect_encoding();
		if (esm_encoding == tools_t::encoding_t::windows_1250)
		{
			this->add_hyperlinks = false;
		}
	}

	if (esm.is_loaded())
		convert_esm();
}

void esm_converter_t::convert_esm()
{
	if (!file_suffix.empty())
		convert_mast();

	convert_cell();
	convert_pgrd();
	convert_anam();
	convert_scvr();
	convert_dnam();
	convert_cndt();
	convert_dial();
	convert_bnam();
	convert_scpt();
	convert_gmst();
	convert_fnam();
	convert_desc();
	convert_text();
	convert_rnam();
	convert_indx();

	if (add_hyperlinks)
	{
		tools_t::add_log("[info] adding hyperlinks\r\n");
	}

	convert_info();
	// convert_gmdt();

	if (create_header)
		make_header();
}

void esm_converter_t::convert_mast()
{
	esm.select_record(0);
	if (esm.get_record().id != "TES3")
		return;

	esm.set_value("MAST");
	while (esm.get_value().exist)
	{
		const auto & prefix = esm.get_value().text.substr(0, esm.get_value().text.find_last_of("."));
		const auto & suffix = esm.get_value().text.substr(esm.get_value().text.rfind("."));
		const auto & new_text = prefix + file_suffix + suffix + '\0';
		convert_record_content(new_text);
		esm.set_next_value("MAST");
	}
}

void esm_converter_t::make_header()
{
	esm.select_record(0);
	if (esm.get_record().id != "TES3")
		return;

	esm.set_value("HEDR");
	std::string hedr = esm.get_value().content;
	std::string version = hedr.substr(0, 4);
	std::string type = hedr.substr(4, 4);
	std::string author = hedr.substr(8, 32);
	std::string description = hedr.substr(40, 256);
	std::string rec_num = hedr.substr(296, 4);

	author.clear();
	author.resize(32);
	description.clear();
	description.resize(256);

	size_t num = esm.get_modified_count();
	rec_num = tools_t::convert_uint_to_string_byte_array(num);

	hedr = version + type + author + description + rec_num;
	convert_record_content(hedr);

	std::string rec_content = esm.get_record().content;
	std::string mast = get_name().full + '\0';
	rec_content += "MAST" + tools_t::convert_uint_to_string_byte_array(mast.size()) + mast;
	std::string data;
	data.resize(8);
	rec_content += "DATA" + tools_t::convert_uint_to_string_byte_array(data.size()) + data;

	size_t rec_size = rec_content.size() - 16;
	rec_content.erase(4, 4);
	rec_content.insert(4, tools_t::convert_uint_to_string_byte_array(rec_size));
	esm.replace_record(rec_content);

	tools_t::add_log("[info] creating new header\r\n");
}

void esm_converter_t::convert_gmdt()
{
	reset_counters();
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id == "TES3")
		{
			esm.set_value("GMDT");
			if (esm.get_value().exist)
			{
				const auto & prefix = esm.get_value().content.substr(0, 24);
				const auto & suffix = esm.get_value().content.substr(88);
				auto val_text = esm.get_value().content.substr(24, 64);
				val_text = tools_t::erase_null_chars(val_text);
				const auto & type = tools_t::rec_type_t::cell;
				std::string new_text;
				if (!make_new_text({ val_text, val_text, type }, new_text))
					continue;

				new_text.resize(64);
				convert_record_content(prefix + new_text + suffix);
			}
		}

		if (esm.get_record().id == "GAME")
		{
			esm.set_value("GMDT");
			if (esm.get_value().exist)
			{
				const auto & suffix = esm.get_value().content.substr(64);
				auto val_text = esm.get_value().content.substr(0, 64);
				val_text = tools_t::erase_null_chars(val_text);
				const auto & type = tools_t::rec_type_t::cell;
				std::string new_text;
				if (!make_new_text({ val_text, val_text, type }, new_text))
					continue;

				new_text.resize(64);
				convert_record_content(new_text + suffix);
			}
		}
	}
	print_log_line(tools_t::rec_type_t::gmdt);
}

void esm_converter_t::convert_cell()
{
	reset_counters();
	const auto & type = tools_t::rec_type_t::cell;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "CELL")
			continue;

		esm.set_value("NAME");
		if (esm.get_value().exist && esm.get_value().text != "")
		{
			const auto & key_text = esm.get_value().text;
			const auto & val_text = esm.get_value().text;
			std::string new_text;
			if (!make_new_text({ key_text, val_text, type }, new_text))
				continue;

			/* null terminated, can't be empty */
			new_text += '\0';
			convert_record_content(new_text);
		}
	}
	print_log_line(tools_t::rec_type_t::cell);
}

void esm_converter_t::convert_pgrd()
{
	reset_counters();
	const auto & type = tools_t::rec_type_t::cell;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "PGRD")
			continue;

		esm.set_value("NAME");
		if (esm.get_value().exist && esm.get_value().text != "")
		{
			const auto & key_text = esm.get_value().text;
			const auto & val_text = esm.get_value().text;
			std::string new_text;
			if (!make_new_text({ key_text, val_text, type }, new_text))
				continue;

			new_text += '\0';
			convert_record_content(new_text);
		}
	}
	print_log_line(tools_t::rec_type_t::pgrd);
}

void esm_converter_t::convert_anam()
{
	reset_counters();
	const auto & type = tools_t::rec_type_t::cell;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "INFO")
			continue;

		esm.set_value("ANAM");
		if (esm.get_value().exist && esm.get_value().text != "")
		{
			const auto & key_text = esm.get_value().text;
			const auto & val_text = esm.get_value().text;
			std::string new_text;
			if (!make_new_text({ key_text, val_text, type }, new_text))
				continue;

			new_text += '\0';
			convert_record_content(new_text);
		}
	}
	print_log_line(tools_t::rec_type_t::anam);
}

void esm_converter_t::convert_scvr()
{
	reset_counters();
	const auto & type = tools_t::rec_type_t::cell;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "INFO")
			continue;

		esm.set_value("SCVR");
		while (esm.get_value().exist)
		{
			/* possible exceptions */
			if (esm.get_value().text.substr(1, 1) == "B")
			{
				const auto & key_text = esm.get_value().text.substr(5);
				const auto & val_text = esm.get_value().text.substr(5);
				std::string new_text;
				if (make_new_text({ key_text, val_text, type }, new_text))
				{
					/* not null terminated */
					new_text = esm.get_value().text.substr(0, 5) + new_text;
					convert_record_content(new_text);
				}
			}
			esm.set_next_value("SCVR");
		}
	}
	print_log_line(tools_t::rec_type_t::scvr);
}

void esm_converter_t::convert_dnam()
{
	reset_counters();
	const auto & type = tools_t::rec_type_t::cell;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id == "CELL" || esm.get_record().id == "NPC_")
		{
			esm.set_value("DNAM");
			while (esm.get_value().exist)
			{
				const auto & key_text = esm.get_value().text;
				const auto & val_text = esm.get_value().text;
				std::string new_text;
				if (make_new_text({ key_text, val_text, type }, new_text))
				{
					new_text += '\0';
					convert_record_content(new_text);
				}
				esm.set_next_value("DNAM");
			}
		}
	}
	print_log_line(tools_t::rec_type_t::dnam);
}

void esm_converter_t::convert_cndt()
{
	reset_counters();
	const auto & type = tools_t::rec_type_t::cell;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "NPC_")
			continue;

		esm.set_value("CNDT");
		while (esm.get_value().exist)
		{
			const auto & key_text = esm.get_value().text;
			const auto & val_text = esm.get_value().text;
			std::string new_text;
			if (make_new_text({ key_text, val_text, type }, new_text))
			{
				new_text += '\0';
				convert_record_content(new_text);
			}
			esm.set_next_value("CNDT");
		}
	}
	print_log_line(tools_t::rec_type_t::cndt);
}

void esm_converter_t::convert_gmst()
{
	reset_counters();
	const auto & type = tools_t::rec_type_t::gmst;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "GMST")
			continue;

		esm.set_key("NAME");
		esm.set_value("STRV");
		if (esm.get_key().exist && esm.get_value().exist &&
		    esm.get_key().text.substr(0, 1) == "s") /* possible exception */
		{
			const auto & key_text = esm.get_key().text;
			const auto & val_text = esm.get_value().text;
			std::string new_text;
			if (!make_new_text({ key_text, val_text, type }, new_text))
				continue;

			/* null terminated only if empty */
			add_null_terminator_if_empty(new_text);
			convert_record_content(new_text);
		}
	}
	print_log_line(tools_t::rec_type_t::gmst);
}

void esm_converter_t::convert_fnam()
{
	reset_counters();
	const auto & type = tools_t::rec_type_t::fnam;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (!tools_t::is_fnam(esm.get_record().id))
			continue;

		esm.set_key("NAME");
		esm.set_value("FNAM");
		if (esm.get_key().exist && esm.get_value().exist && esm.get_key().text != "player")
		{
			const auto & key_text = esm.get_record().id + "^" + esm.get_key().text;
			const auto & val_text = esm.get_value().text;
			std::string new_text;
			if (!make_new_text({ key_text, val_text, type }, new_text))
				continue;

			/* null terminated, don't exist if empty */
			new_text += '\0';
			convert_record_content(new_text);
		}
	}
	print_log_line(tools_t::rec_type_t::fnam);
}

void esm_converter_t::convert_desc()
{
	reset_counters();
	const auto & type = tools_t::rec_type_t::desc;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id == "BSGN" || esm.get_record().id == "CLAS" || esm.get_record().id == "RACE")
		{
			esm.set_key("NAME");
			esm.set_value("DESC");
			if (esm.get_key().exist && esm.get_value().exist)
			{
				const auto & key_text = esm.get_record().id + "^" + esm.get_key().text;
				const auto & val_text = esm.get_value().text;
				std::string new_text;
				if (!make_new_text({ key_text, val_text, type }, new_text))
					continue;

				if (esm.get_record().id == "BSGN")
				{
					/* null terminated, don't exist if empty */
					new_text += '\0';
					convert_record_content(new_text);
				}

				if (esm.get_record().id == "CLAS" || esm.get_record().id == "RACE")
				{
					/* not null terminated, don't exist if empty */
					add_null_terminator_if_empty(new_text);
					convert_record_content(new_text);
				}
			}
		}
	}
	print_log_line(tools_t::rec_type_t::desc);
}

void esm_converter_t::convert_text()
{
	reset_counters();
	const auto & type = tools_t::rec_type_t::text;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "BOOK")
			continue;

		esm.set_key("NAME");
		esm.set_value("TEXT");
		if (esm.get_key().exist && esm.get_value().exist)
		{
			const auto & key_text = esm.get_key().text;
			const auto & val_text = esm.get_value().text;
			std::string new_text;
			if (!make_new_text({ key_text, val_text, type }, new_text))
				continue;

			/* not null terminated, don't exist if empty */
			add_null_terminator_if_empty(new_text);
			convert_record_content(new_text);
		}
	}
	print_log_line(tools_t::rec_type_t::text);
}

void esm_converter_t::convert_rnam()
{
	reset_counters();
	const auto & type = tools_t::rec_type_t::rnam;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "FACT")
			continue;

		esm.set_key("NAME");
		esm.set_value("RNAM");
		if (!esm.get_key().exist)
			continue;

		while (esm.get_value().exist)
		{
			const auto & key_text = esm.get_key().text + "^" + std::to_string(esm.get_value().counter);
			const auto & val_text = esm.get_value().text;
			std::string new_text;
			if (make_new_text({ key_text, val_text, type }, new_text))
			{
				/* null terminated up to 32 */
				new_text.resize(32);
				convert_record_content(new_text);
			}
			esm.set_next_value("RNAM");
		}
	}
	print_log_line(tools_t::rec_type_t::rnam);
}

void esm_converter_t::convert_indx()
{
	reset_counters();
	const auto & type = tools_t::rec_type_t::indx;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id == "SKIL" || esm.get_record().id == "MGEF")
		{
			esm.set_key("INDX");
			esm.set_value("DESC");
			if (esm.get_key().exist && esm.get_value().exist)
			{
				const auto & key_text = esm.get_record().id + "^" + tools_t::get_indx(esm.get_key().content);
				const auto & val_text = esm.get_value().text;
				std::string new_text;
				if (!make_new_text({ key_text, val_text, type }, new_text))
					continue;

				/* not null terminated, don't exist if empty */
				add_null_terminator_if_empty(new_text);
				convert_record_content(new_text);
			}
		}
	}
	print_log_line(tools_t::rec_type_t::indx);
}

void esm_converter_t::convert_dial()
{
	reset_counters();
	const auto & type = tools_t::rec_type_t::dial;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "DIAL")
			continue;

		esm.set_key("DATA");
		esm.set_value("NAME");
		if (tools_t::get_dialog_type(esm.get_key().content) == "T" && esm.get_value().exist)
		{
			const auto & key_text = esm.get_value().text;
			const auto & val_text = esm.get_value().text;
			std::string new_text;
			if (!make_new_text({ key_text, val_text, type }, new_text))
				continue;

			/* null terminated */
			new_text += '\0';
			convert_record_content(new_text);
		}
	}
	print_log_line(tools_t::rec_type_t::dial);
}

void esm_converter_t::convert_info()
{
	std::string key_prefix;
	size_t dial_index = 0;

	reset_counters();
	const auto & type = tools_t::rec_type_t::info;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id == "DIAL")
		{
			esm.set_key("DATA");
			esm.set_value("NAME");
			if (esm.get_key().exist && esm.get_value().exist)
			{
				key_prefix = tools_t::get_dialog_type(esm.get_key().content) + "^" + esm.get_value().text;
				dial_index = i;
			}
		}

		if (esm.get_record().id == "INFO")
		{
			esm.set_key("INAM");
			esm.set_value("NAME");
			if (esm.get_key().exist && esm.get_value().exist)
			{
				const auto & key_text = key_prefix + "^" + esm.get_key().text;
				const auto & val_text = esm.get_value().text;
				std::string new_text;
				if (make_new_text({ key_text, val_text, type }, new_text))
				{
					/* not null terminated, don't exist if empty */
					add_null_terminator_if_empty(new_text);
					convert_record_content(new_text);
					esm.set_modified(dial_index);
				}
			}
		}
	}
	print_log_line(tools_t::rec_type_t::info);
}

void esm_converter_t::convert_bnam()
{
	size_t dial_index = 0;

	reset_counters();
	const auto & type = tools_t::rec_type_t::bnam;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id == "DIAL")
		{
			esm.set_key("DATA");
			esm.set_value("NAME");
			if (esm.get_key().exist && esm.get_value().exist)
			{
				dial_index = i;
			}
		}

		if (esm.get_record().id == "INFO")
		{
			esm.set_key("INAM");
			esm.set_value("BNAM");
			if (esm.get_key().exist && esm.get_value().exist)
			{
				const auto & key_text = esm.get_key().text;
				const auto & val_text = esm.get_value().text;

				const auto & script_name = key_text;
				const auto & file_name = get_name().full;
				const auto & old_script = val_text;

				counter_all++;
				script_parser_t parser(type, merger, script_name, file_name, old_script);

				std::string new_text = parser.get_new_script();
				if (is_identical(val_text, new_text))
					continue;

				convert_record_content(new_text);
				esm.set_modified(dial_index);
			}
		}
	}
	print_log_line(tools_t::rec_type_t::bnam);
}

void esm_converter_t::convert_scpt()
{
	std::string old_scdt;
	reset_counters();
	const auto & type = tools_t::rec_type_t::sctx;
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id != "SCPT")
			continue;

		esm.set_value("SCDT");
		if (esm.get_value().exist)
		{
			old_scdt = esm.get_value().content;
		}
		else
		{
			old_scdt.clear();
		}

		esm.set_key("SCHD");
		esm.set_value("SCTX");
		if (esm.get_key().exist && esm.get_value().exist)
		{
			const auto & key_text = esm.get_key().text;
			const auto & val_text = esm.get_value().text;

			const auto & script_name = key_text;
			const auto & file_name = get_name().full;
			const auto & old_script = val_text;

			counter_all++;
			script_parser_t parser(type, merger, script_name, file_name, old_script, old_scdt);

			const auto & new_text = parser.get_new_script();
			if (is_identical(val_text, new_text))
				continue;

			convert_record_content(new_text);

			{
				/* compiled script data */
				esm.set_value("SCDT");
				const auto & new_text = parser.get_new_scdt();
				convert_record_content(new_text);
			}

			{
				/* compiled script data size in script name */
				esm.set_value("SCHD");
				auto new_text = esm.get_value().content;
				new_text.erase(44, 4);
				new_text.insert(44, tools_t::convert_uint_to_string_byte_array(parser.get_new_scdt().size()));
				convert_record_content(new_text);
			}
		}
	}
	print_log_line(tools_t::rec_type_t::sctx);
}

void esm_converter_t::reset_counters()
{
	counter_converted = 0;
	counter_identical = 0;
	counter_unchanged = 0;
	counter_all = 0;
	counter_added = 0;
}

bool esm_converter_t::make_new_text(const tools_t::entry_t & entry, std::string & new_text)
{
	counter_all++;
	new_text.clear();
	auto * found = merger.get_dict().at(entry.type).find(entry.key_text);
	if (found)
	{
		new_text = found->new_text;
		return !is_identical(entry.val_text, new_text);
	}

	counter_unchanged++;
	return false;
}

bool esm_converter_t::is_identical(const std::string & old_text, const std::string & new_text)
{
	if (old_text == new_text)
	{
		counter_identical++;
		return true;
	}

	return false;
}

void esm_converter_t::add_null_terminator_if_empty(std::string & new_text)
{
	if (new_text.empty())
		new_text = '\0';
}

void esm_converter_t::convert_record_content(const std::string & new_text)
{
	std::string rec_content = esm.get_record().content;
	rec_content.erase(esm.get_value().pos + 8, esm.get_value().size);
	rec_content.insert(esm.get_value().pos + 8, new_text);
	rec_content.erase(esm.get_value().pos + 4, 4);
	rec_content.insert(esm.get_value().pos + 4, tools_t::convert_uint_to_string_byte_array(new_text.size()));
	size_t rec_size = rec_content.size() - 16;
	rec_content.erase(4, 4);
	rec_content.insert(4, tools_t::convert_uint_to_string_byte_array(rec_size));
	esm.replace_record(rec_content);
	counter_converted++;
}

void esm_converter_t::print_log_line(const tools_t::rec_type_t type)
{
	std::string line = tools_t::type_to_str(type) + ": converted=" + std::to_string(counter_converted) +
	                   ", identical=" + std::to_string(counter_identical) +
	                   ", unchanged=" + std::to_string(counter_unchanged) + ", total=" + std::to_string(counter_all) +
	                   "\r\n";

	tools_t::add_log(line);
}

tools_t::encoding_t esm_converter_t::detect_encoding()
{
	for (size_t i = 0; i < esm.get_records().size(); ++i)
	{
		esm.select_record(i);
		if (esm.get_record().id == "INFO")
			esm.set_value("NAME");

		if (detect_windows_1250_encoding(esm.get_value().text))
		{
			tools_t::add_log("[warn] windows-1250 encoding detected\r\n");
			tools_t::add_log(esm.get_value().text + "\r\n", true);
			return tools_t::encoding_t::windows_1250;
		}
	}
	return tools_t::encoding_t::unknown;
}

bool esm_converter_t::detect_windows_1250_encoding(const std::string & val_text)
{
	// 156 œ ś
	// 159 Ÿ ź
	// 179 ³ ł
	// 185 ¹ ą
	// 191 ¿ ż
	// 230 æ ć
	// 234 ê ę
	// 241 ñ ń
	// 243 ó ó <- found in Tamriel Rebuilt

	std::ostringstream ss;
	ss << static_cast<char>(156) << static_cast<char>(159) << static_cast<char>(179) << static_cast<char>(185)
	   << static_cast<char>(191) << static_cast<char>(230) << static_cast<char>(234) << static_cast<char>(241);

	return val_text.find_first_of(ss.str()) != std::string::npos;
}
