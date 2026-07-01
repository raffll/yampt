#include <catch2/catch_all.hpp>
#include <converter/esm_converter.hpp>
#include <io/dict_writer.hpp>
#include <random>

static std::string get_temp_path(const std::string & filename)
{
	return (std::filesystem::temp_directory_path() / filename).string();
}

static std::string make_sub_record(const std::string & sub_id, const std::string & content)
{
	std::string result;
	result += sub_id;
	result += tools_t::convert_uint_to_string_byte_array(content.size());
	result += content;
	return result;
}

static std::string make_record(const std::string & rec_id, const std::string & sub_records)
{
	std::string header;
	header += rec_id;
	header += tools_t::convert_uint_to_string_byte_array(sub_records.size());
	header += tools_t::convert_uint_to_string_byte_array(0);
	header += tools_t::convert_uint_to_string_byte_array(0);
	return header + sub_records;
}

static std::string make_tes3_record()
{
	return make_record("TES3", make_sub_record("HEDR", std::string(300, '\0')));
}

static dict_merger_t make_merger(
    const std::vector<std::tuple<tools_t::rec_type_t, std::string, std::string, std::string>> & entries)
{
	const auto dict_path = get_temp_path("yampt_test_full_conv_dict.json");
	tools_t::reset_log();

	tools_t::dict_t dict = tools_t::initialize_dict();
	for (const auto & [record_type, key_text, old_text, new_text] : entries)
		dict.at(record_type).insert({ key_text, old_text, new_text, status_t::translated });

	dict_writer_t::write(dict, dict_path);

	tools_t::reset_log();
	dict_merger_t merger({ dict_path });
	tools_t::reset_log();
	std::filesystem::remove(dict_path);
	return merger;
}

static void write_esm_file(const std::string & content, const std::string & path)
{
	std::ofstream file(path, std::ios::binary);
	file.write(content.data(), content.size());
}

static std::string run_converter(const std::string & esm_content, const dict_merger_t & merger)
{
	const auto esm_path = get_temp_path("yampt_test_full_conv.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());
	return converter.get_records().back().content;
}

static std::vector<tools_t::record_t> run_converter_all(const std::string & esm_content, const dict_merger_t & merger)
{
	const auto esm_path = get_temp_path("yampt_test_full_conv.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());
	return converter.get_records();
}

TEST_CASE("esm_converter_t::convert_scpt, SCTX with matching cell", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Balmora", "Balmora", "Balmora PL" },
	    });

	std::string old_sctx = "if ( GetPCCell \"Balmora\" == 1 )\r\nset x to 1\r\nendif";
	std::string old_scdt(32, '\x00');

	std::string schd_content(48, '\0');
	std::string script_name = "TestScript";
	schd_content.replace(0, script_name.size(), script_name);
	schd_content.erase(44, 4);
	schd_content.insert(44, tools_t::convert_uint_to_string_byte_array(old_scdt.size()));

	auto scpt_body =
	    make_sub_record("SCHD", schd_content) + make_sub_record("SCDT", old_scdt) + make_sub_record("SCTX", old_sctx);
	auto esm_content = make_tes3_record() + make_record("SCPT", scpt_body);

	const auto & records = run_converter_all(esm_content, merger);
	const auto & scpt_content = records[1].content;
	REQUIRE(scpt_content.find("Balmora PL") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_scpt, SCTX no match leaves unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Ald-ruhn", "Ald-ruhn", "Ald-ruhn PL" },
	    });

	std::string old_sctx = "if ( GetPCCell \"Balmora\" == 1 )\r\nset x to 1\r\nendif";
	std::string old_scdt(32, '\x00');

	std::string schd_content(48, '\0');
	std::string script_name = "TestScript2";
	schd_content.replace(0, script_name.size(), script_name);
	schd_content.erase(44, 4);
	schd_content.insert(44, tools_t::convert_uint_to_string_byte_array(old_scdt.size()));

	auto scpt_body =
	    make_sub_record("SCHD", schd_content) + make_sub_record("SCDT", old_scdt) + make_sub_record("SCTX", old_sctx);
	auto esm_content = make_tes3_record() + make_record("SCPT", scpt_body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_bnam, INFO script with AddTopic", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::dial, "background", "background", "tlo" },
	    });

	std::string topic_type(1, '\0');
	auto dial_body = make_sub_record("NAME", std::string("topic\0", 6)) + make_sub_record("DATA", topic_type);
	auto dial_record = make_record("DIAL", dial_body);

	auto info_body = make_sub_record("INAM", std::string("info_01\0", 8)) +
	                 make_sub_record("BNAM", std::string("AddTopic \"background\""));
	auto info_record = make_record("INFO", info_body);

	auto esm_content = make_tes3_record() + dial_record + info_record;

	const auto & records = run_converter_all(esm_content, merger);
	const auto & info_content = records[2].content;
	REQUIRE(info_content.find("AddTopic \"tlo\"") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_bnam, no match leaves script unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::dial, "unrelated", "unrelated", "niezwiazany" },
	    });

	std::string topic_type(1, '\0');
	auto dial_body = make_sub_record("NAME", std::string("topic\0", 6)) + make_sub_record("DATA", topic_type);
	auto dial_record = make_record("DIAL", dial_body);

	auto info_body =
	    make_sub_record("INAM", std::string("info_02\0", 8)) + make_sub_record("BNAM", std::string("set x to 1"));
	auto info_record = make_record("INFO", info_body);

	auto esm_content = make_tes3_record() + dial_record + info_record;

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[2].modified == false);
}

TEST_CASE("esm_converter_t::convert_cell, interior cell name converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell,
	          "Balmora, Guild of Fighters",
	          "Balmora, Guild of Fighters",
	          "Balmora, Gildia Wojownikow" },
	    });

	auto body = make_sub_record("NAME", std::string("Balmora, Guild of Fighters\0", 27));
	auto esm_content = make_tes3_record() + make_record("CELL", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Balmora, Gildia Wojownikow") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_cell, no match leaves unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Ald-ruhn", "Ald-ruhn", "Ald-ruhn PL" },
	    });

	auto body = make_sub_record("NAME", std::string("Balmora\0", 8));
	auto esm_content = make_tes3_record() + make_record("CELL", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_cell, empty NAME is skipped", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "", "", "Something" },
	    });

	auto body = make_sub_record("NAME", std::string("\0", 1));
	auto esm_content = make_tes3_record() + make_record("CELL", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_fnam, NPC display name converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::fnam, "NPC_^hlaalu_guard", "Guard", "Straznik" },
	    });

	auto body =
	    make_sub_record("NAME", std::string("hlaalu_guard\0", 13)) + make_sub_record("FNAM", std::string("Guard\0", 6));
	auto esm_content = make_tes3_record() + make_record("NPC_", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Straznik") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_fnam, SPEL name converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::fnam, "SPEL^fireball", "Fireball", "Kula Ognia" },
	    });

	auto body =
	    make_sub_record("NAME", std::string("fireball\0", 9)) + make_sub_record("FNAM", std::string("Fireball\0", 9));
	auto esm_content = make_tes3_record() + make_record("SPEL", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Kula Ognia") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_fnam, no match leaves unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::fnam, "NPC_^other_npc", "Other", "Inny" },
	    });

	auto body =
	    make_sub_record("NAME", std::string("hlaalu_guard\0", 13)) + make_sub_record("FNAM", std::string("Guard\0", 6));
	auto esm_content = make_tes3_record() + make_record("NPC_", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_fnam, player NPC excluded", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::fnam, "NPC_^player", "Player", "Gracz" },
	    });

	auto body =
	    make_sub_record("NAME", std::string("player\0", 7)) + make_sub_record("FNAM", std::string("Player\0", 7));
	auto esm_content = make_tes3_record() + make_record("NPC_", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_info, dialogue response converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::info, "T^background^info_01", "I am from Morrowind.", "Jestem z Morrowind." },
	    });

	std::string topic_type(1, '\0');
	auto dial_body = make_sub_record("NAME", std::string("background\0", 11)) + make_sub_record("DATA", topic_type);
	auto dial_record = make_record("DIAL", dial_body);

	auto info_body = make_sub_record("INAM", std::string("info_01\0", 8)) +
	                 make_sub_record("NAME", std::string("I am from Morrowind.", 20));
	auto info_record = make_record("INFO", info_body);

	auto esm_content = make_tes3_record() + dial_record + info_record;

	const auto & records = run_converter_all(esm_content, merger);
	const auto & info_content = records[2].content;
	REQUIRE(info_content.find("Jestem z Morrowind.") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_info, no match leaves unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::info, "T^background^info_99", "Other text", "Inny tekst" },
	    });

	std::string topic_type(1, '\0');
	auto dial_body = make_sub_record("NAME", std::string("background\0", 11)) + make_sub_record("DATA", topic_type);
	auto dial_record = make_record("DIAL", dial_body);

	auto info_body = make_sub_record("INAM", std::string("info_01\0", 8)) +
	                 make_sub_record("NAME", std::string("I am from Morrowind.", 20));
	auto info_record = make_record("INFO", info_body);

	auto esm_content = make_tes3_record() + dial_record + info_record;

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[2].modified == false);
}

TEST_CASE("esm_converter_t::convert_dial, topic name converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::dial, "background", "background", "tlo" },
	    });

	std::string topic_type(1, '\x00');
	auto dial_body = make_sub_record("NAME", std::string("background\0", 11)) + make_sub_record("DATA", topic_type);
	auto esm_content = make_tes3_record() + make_record("DIAL", dial_body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("tlo") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_dial, non-topic type is skipped", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::dial, "greeting 1", "greeting 1", "powitanie 1" },
	    });

	std::string greeting_type(1, '\x02');
	auto dial_body = make_sub_record("NAME", std::string("greeting 1\0", 11)) + make_sub_record("DATA", greeting_type);
	auto esm_content = make_tes3_record() + make_record("DIAL", dial_body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_dial, no match leaves unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::dial, "other topic", "other topic", "inny temat" },
	    });

	std::string topic_type(1, '\x00');
	auto dial_body = make_sub_record("NAME", std::string("background\0", 11)) + make_sub_record("DATA", topic_type);
	auto esm_content = make_tes3_record() + make_record("DIAL", dial_body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_gmst, string setting value replaced", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::gmst, "sDefaultCellname", "Wilderness", "Dzicz" },
	    });

	auto body = make_sub_record("NAME", std::string("sDefaultCellname\0", 17)) +
	            make_sub_record("STRV", std::string("Wilderness", 10));
	auto esm_content = make_tes3_record() + make_record("GMST", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Dzicz") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_gmst, non-string setting not modified", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::gmst, "iMaxLevel", "100", "200" },
	    });

	auto body =
	    make_sub_record("NAME", std::string("iMaxLevel\0", 10)) + make_sub_record("STRV", std::string("100", 3));
	auto esm_content = make_tes3_record() + make_record("GMST", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_gmst, no match leaves unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::gmst, "sOtherSetting", "Other", "Inne" },
	    });

	auto body = make_sub_record("NAME", std::string("sDefaultCellname\0", 17)) +
	            make_sub_record("STRV", std::string("Wilderness", 10));
	auto esm_content = make_tes3_record() + make_record("GMST", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_desc, BSGN description converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::desc, "BSGN^The Lady", "Fortify Personality", "Wzmocnienie Osobowosci" },
	    });

	auto body = make_sub_record("NAME", std::string("The Lady\0", 9)) +
	            make_sub_record("DESC", std::string("Fortify Personality", 19));
	auto esm_content = make_tes3_record() + make_record("BSGN", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Wzmocnienie Osobowosci") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_desc, CLAS description converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::desc, "CLAS^Warrior", "Favors strength", "Preferuje sile" },
	    });

	auto body = make_sub_record("NAME", std::string("Warrior\0", 8)) +
	            make_sub_record("DESC", std::string("Favors strength", 15));
	auto esm_content = make_tes3_record() + make_record("CLAS", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Preferuje sile") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_desc, RACE description converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::desc, "RACE^Dark Elf", "The dark elves", "Ciemne elfy" },
	    });

	auto body = make_sub_record("NAME", std::string("Dark Elf\0", 9)) +
	            make_sub_record("DESC", std::string("The dark elves", 14));
	auto esm_content = make_tes3_record() + make_record("RACE", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Ciemne elfy") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_desc, no match leaves unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::desc, "BSGN^The Warrior", "Fortify Attack", "Wzmocnienie Ataku" },
	    });

	auto body = make_sub_record("NAME", std::string("The Lady\0", 9)) +
	            make_sub_record("DESC", std::string("Fortify Personality", 19));
	auto esm_content = make_tes3_record() + make_record("BSGN", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_text, BOOK text converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::text, "book_001", "Once upon a time", "Dawno dawno temu" },
	    });

	auto body = make_sub_record("NAME", std::string("book_001\0", 9)) +
	            make_sub_record("TEXT", std::string("Once upon a time", 16));
	auto esm_content = make_tes3_record() + make_record("BOOK", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Dawno dawno temu") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_text, no match leaves unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::text, "book_999", "Other book text", "Inny tekst ksiazki" },
	    });

	auto body = make_sub_record("NAME", std::string("book_001\0", 9)) +
	            make_sub_record("TEXT", std::string("Once upon a time", 16));
	auto esm_content = make_tes3_record() + make_record("BOOK", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_rnam, faction rank converted", "[u]")
{
	std::string rank_name(32, '\0');
	std::string("Associate").copy(rank_name.data(), 9);

	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::rnam, "Fighters Guild^0", "Associate", "Stowarzyszony" },
	    });

	auto body = make_sub_record("NAME", std::string("Fighters Guild\0", 15)) + make_sub_record("RNAM", rank_name);
	auto esm_content = make_tes3_record() + make_record("FACT", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Stowarzyszony") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_rnam, no match leaves unchanged", "[u]")
{
	std::string rank_name(32, '\0');
	std::string("Associate").copy(rank_name.data(), 9);

	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::rnam, "Thieves Guild^0", "Toad", "Ropucha" },
	    });

	auto body = make_sub_record("NAME", std::string("Fighters Guild\0", 15)) + make_sub_record("RNAM", rank_name);
	auto esm_content = make_tes3_record() + make_record("FACT", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_indx, SKIL description converted", "[u]")
{
	std::string indx_content(4, '\0');
	indx_content[0] = 5;

	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::indx, "SKIL^005", "Destruction magic", "Magia Destrukcji" },
	    });

	auto body = make_sub_record("INDX", indx_content) + make_sub_record("DESC", std::string("Destruction magic", 17));
	auto esm_content = make_tes3_record() + make_record("SKIL", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Magia Destrukcji") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_indx, MGEF description converted", "[u]")
{
	std::string indx_content(4, '\0');
	indx_content[0] = 3;

	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::indx, "MGEF^003", "Absorb Fatigue", "Absorpcja Zmeczenia" },
	    });

	auto body = make_sub_record("INDX", indx_content) + make_sub_record("DESC", std::string("Absorb Fatigue", 14));
	auto esm_content = make_tes3_record() + make_record("MGEF", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Absorpcja Zmeczenia") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_indx, no match leaves unchanged", "[u]")
{
	std::string indx_content(4, '\0');
	indx_content[0] = 5;

	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::indx, "SKIL^010", "Enchant", "Zaklinanie" },
	    });

	auto body = make_sub_record("INDX", indx_content) + make_sub_record("DESC", std::string("Destruction magic", 17));
	auto esm_content = make_tes3_record() + make_record("SKIL", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_pgrd, cell name in pathgrid converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Balmora", "Balmora", "Balmora PL" },
	    });

	auto body = make_sub_record("NAME", std::string("Balmora\0", 8));
	auto esm_content = make_tes3_record() + make_record("PGRD", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Balmora PL") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_pgrd, empty NAME is skipped", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "", "", "Something" },
	    });

	auto body = make_sub_record("NAME", std::string("\0", 1));
	auto esm_content = make_tes3_record() + make_record("PGRD", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_pgrd, no match leaves unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Vivec", "Vivec", "Vivec PL" },
	    });

	auto body = make_sub_record("NAME", std::string("Balmora\0", 8));
	auto esm_content = make_tes3_record() + make_record("PGRD", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_anam, INFO cell filter converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Balmora", "Balmora", "Balmora PL" },
	    });

	auto body =
	    make_sub_record("INAM", std::string("info_01\0", 8)) + make_sub_record("ANAM", std::string("Balmora\0", 8));
	auto esm_content = make_tes3_record() + make_record("INFO", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Balmora PL") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_anam, empty ANAM is skipped", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "", "", "Something" },
	    });

	auto body = make_sub_record("INAM", std::string("info_01\0", 8)) + make_sub_record("ANAM", std::string("\0", 1));
	auto esm_content = make_tes3_record() + make_record("INFO", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_anam, no match leaves unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Vivec", "Vivec", "Vivec PL" },
	    });

	auto body =
	    make_sub_record("INAM", std::string("info_01\0", 8)) + make_sub_record("ANAM", std::string("Balmora\0", 8));
	auto esm_content = make_tes3_record() + make_record("INFO", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_scvr, condition cell name converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Balmora", "Balmora", "Balmora PL" },
	    });

	std::string scvr_content = "5BLX0Balmora";
	auto body = make_sub_record("INAM", std::string("info_01\0", 8)) + make_sub_record("SCVR", scvr_content);
	auto esm_content = make_tes3_record() + make_record("INFO", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Balmora PL") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_scvr, non-B type prefix is skipped", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Balmora", "Balmora", "Balmora PL" },
	    });

	std::string scvr_content = "5ALX0Balmora";
	auto body = make_sub_record("INAM", std::string("info_01\0", 8)) + make_sub_record("SCVR", scvr_content);
	auto esm_content = make_tes3_record() + make_record("INFO", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_scvr, no match leaves unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Vivec", "Vivec", "Vivec PL" },
	    });

	std::string scvr_content = "5BLX0Balmora";
	auto body = make_sub_record("INAM", std::string("info_01\0", 8)) + make_sub_record("SCVR", scvr_content);
	auto esm_content = make_tes3_record() + make_record("INFO", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_dnam, door destination in CELL converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell,
	          "Balmora, Guild of Fighters",
	          "Balmora, Guild of Fighters",
	          "Balmora, Gildia Wojownikow" },
	    });

	auto body = make_sub_record("NAME", std::string("some_cell\0", 10)) +
	            make_sub_record("DNAM", std::string("Balmora, Guild of Fighters\0", 27));
	auto esm_content = make_tes3_record() + make_record("CELL", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Balmora, Gildia Wojownikow") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_dnam, DNAM in NPC_ record converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Vivec", "Vivec", "Vivec PL" },
	    });

	auto body =
	    make_sub_record("NAME", std::string("npc_id\0", 7)) + make_sub_record("DNAM", std::string("Vivec\0", 6));
	auto esm_content = make_tes3_record() + make_record("NPC_", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Vivec PL") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_dnam, no match leaves unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Ald-ruhn", "Ald-ruhn", "Ald-ruhn PL" },
	    });

	auto body =
	    make_sub_record("NAME", std::string("some_cell\0", 10)) + make_sub_record("DNAM", std::string("Balmora\0", 8));
	auto esm_content = make_tes3_record() + make_record("CELL", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_cndt, NPC escort cell converted", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Vivec", "Vivec", "Vivec PL" },
	    });

	auto body =
	    make_sub_record("NAME", std::string("npc_id\0", 7)) + make_sub_record("CNDT", std::string("Vivec\0", 6));
	auto esm_content = make_tes3_record() + make_record("NPC_", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Vivec PL") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_cndt, no match leaves unchanged", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Ald-ruhn", "Ald-ruhn", "Ald-ruhn PL" },
	    });

	auto body =
	    make_sub_record("NAME", std::string("npc_id\0", 7)) + make_sub_record("CNDT", std::string("Vivec\0", 6));
	auto esm_content = make_tes3_record() + make_record("NPC_", body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t, only translated status is applied", "[u]")
{
	const std::vector<status_t> rejected_statuses = {
		status_t::untranslated, status_t::missing,     status_t::duplicate, status_t::mismatch,   status_t::heuristic,
		status_t::to_verify,    status_t::adapted,     status_t::changed,   status_t::outdated,   status_t::reused,
		status_t::ambiguous,    status_t::in_progress, status_t::model,     status_t::propagated, status_t::error,
	};

	for (const auto & status : rejected_statuses)
	{
		const auto dict_path = get_temp_path("yampt_test_full_conv_dict.json");
		tools_t::reset_log();

		tools_t::dict_t dict = tools_t::initialize_dict();
		dict.at(tools_t::rec_type_t::cell).insert({ "Balmora", "Balmora", "Balmora PL", status });
		dict_writer_t::write(dict, dict_path);

		tools_t::reset_log();
		dict_merger_t merger({ dict_path });
		tools_t::reset_log();
		std::filesystem::remove(dict_path);

		auto cell_body = make_sub_record("NAME", std::string("Balmora\0", 8));
		auto esm_content = make_tes3_record() + make_record("CELL", cell_body);

		const auto esm_path = get_temp_path("yampt_test_full_conv.esm");
		write_esm_file(esm_content, esm_path);
		tools_t::reset_log();

		esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
		std::filesystem::remove(esm_path);

		REQUIRE(converter.is_loaded());
		REQUIRE(converter.get_records()[1].modified == false);
	}
}

TEST_CASE("esm_converter_t, translated status is applied", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Balmora", "Balmora", "Balmora PL" },
	    });

	auto cell_body = make_sub_record("NAME", std::string("Balmora\0", 8));
	auto esm_content = make_tes3_record() + make_record("CELL", cell_body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Balmora PL") != std::string::npos);
}

TEST_CASE("esm_converter_t, identical old and new text is not applied", "[u]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Balmora", "Balmora", "Balmora" },
	    });

	auto cell_body = make_sub_record("NAME", std::string("Balmora\0", 8));
	auto esm_content = make_tes3_record() + make_record("CELL", cell_body);

	const auto & records = run_converter_all(esm_content, merger);
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t, round-trip yields dict new_text for all record types", "[u]")
{
	const auto iteration = GENERATE(range(0, 100));

	struct round_trip_case_t
	{
		tools_t::rec_type_t record_type;
		std::string key_text;
		std::string old_text;
		std::string new_text;
		std::string esm_record;
	};

	const auto make_cell_case = [](int variant) -> round_trip_case_t
	{
		const auto suffix = std::to_string(variant);
		const auto name = "TestCell_" + suffix;
		const auto translated = "Komorka_" + suffix;
		auto body = make_sub_record("NAME", name + '\0');
		return { tools_t::rec_type_t::cell, name, name, translated, make_record("CELL", body) };
	};

	const auto make_fnam_case = [](int variant) -> round_trip_case_t
	{
		const auto suffix = std::to_string(variant);
		const auto npc_id = "npc_test_" + suffix;
		const auto display = "Guard_" + suffix;
		const auto translated = "Straznik_" + suffix;
		const auto key = "NPC_^" + npc_id;
		auto body = make_sub_record("NAME", npc_id + '\0') + make_sub_record("FNAM", display + '\0');
		return { tools_t::rec_type_t::fnam, key, display, translated, make_record("NPC_", body) };
	};

	const auto make_dial_case = [](int variant) -> round_trip_case_t
	{
		const auto suffix = std::to_string(variant);
		const auto topic = "topic_" + suffix;
		const auto translated = "temat_" + suffix;
		std::string topic_type(1, '\x00');
		auto body = make_sub_record("NAME", topic + '\0') + make_sub_record("DATA", topic_type);
		return { tools_t::rec_type_t::dial, topic, topic, translated, make_record("DIAL", body) };
	};

	const auto make_gmst_case = [](int variant) -> round_trip_case_t
	{
		const auto suffix = std::to_string(variant);
		const auto setting_name = "sSetting_" + suffix;
		const auto value = "Value_" + suffix;
		const auto translated = "Wartosc_" + suffix;
		auto body = make_sub_record("NAME", setting_name + '\0') + make_sub_record("STRV", value);
		return { tools_t::rec_type_t::gmst, setting_name, value, translated, make_record("GMST", body) };
	};

	const auto make_text_case = [](int variant) -> round_trip_case_t
	{
		const auto suffix = std::to_string(variant);
		const auto book_id = "book_" + suffix;
		const auto text = "BookText_" + suffix;
		const auto translated = "TekstKsiazki_" + suffix;
		auto body = make_sub_record("NAME", book_id + '\0') + make_sub_record("TEXT", text);
		return { tools_t::rec_type_t::text, book_id, text, translated, make_record("BOOK", body) };
	};

	const auto make_desc_case = [](int variant) -> round_trip_case_t
	{
		const auto suffix = std::to_string(variant);
		const auto race_id = "Race_" + suffix;
		const auto desc = "Description_" + suffix;
		const auto translated = "Opis_" + suffix;
		const auto key = "RACE^" + race_id;
		auto body = make_sub_record("NAME", race_id + '\0') + make_sub_record("DESC", desc);
		return { tools_t::rec_type_t::desc, key, desc, translated, make_record("RACE", body) };
	};

	const auto make_rnam_case = [](int variant) -> round_trip_case_t
	{
		const auto suffix = std::to_string(variant);
		const auto faction = "Faction_" + suffix;
		const auto rank = "Rank_" + suffix;
		const auto translated = "Ranga_" + suffix;
		const auto key = faction + "^0";
		std::string rank_content(32, '\0');
		rank.copy(rank_content.data(), rank.size());
		auto body = make_sub_record("NAME", faction + '\0') + make_sub_record("RNAM", rank_content);
		return { tools_t::rec_type_t::rnam, key, rank, translated, make_record("FACT", body) };
	};

	const auto make_indx_case = [](int variant) -> round_trip_case_t
	{
		const auto index_value = variant % 27;
		const auto suffix = std::to_string(index_value);
		std::string indx_content(4, '\0');
		indx_content[0] = static_cast<char>(index_value);
		const auto desc = "SkillDesc_" + suffix;
		const auto translated = "OpisUmiejetnosci_" + suffix;
		char key_buf[16];
		std::snprintf(key_buf, sizeof(key_buf), "SKIL^%03d", index_value);
		const auto key = std::string(key_buf);
		auto body = make_sub_record("INDX", indx_content) + make_sub_record("DESC", desc);
		return { tools_t::rec_type_t::indx, key, desc, translated, make_record("SKIL", body) };
	};

	const auto make_pgrd_case = [](int variant) -> round_trip_case_t
	{
		const auto suffix = std::to_string(variant);
		const auto cell_name = "PgrdCell_" + suffix;
		const auto translated = "SciePgrd_" + suffix;
		auto body = make_sub_record("NAME", cell_name + '\0');
		return { tools_t::rec_type_t::cell, cell_name, cell_name, translated, make_record("PGRD", body) };
	};

	const auto make_dnam_case = [](int variant) -> round_trip_case_t
	{
		const auto suffix = std::to_string(variant);
		const auto cell_name = "DoorDest_" + suffix;
		const auto translated = "CelDrzwi_" + suffix;
		auto body = make_sub_record("NAME", std::string("some_cell\0", 10)) + make_sub_record("DNAM", cell_name + '\0');
		return { tools_t::rec_type_t::cell, cell_name, cell_name, translated, make_record("CELL", body) };
	};

	const auto make_anam_case = [](int variant) -> round_trip_case_t
	{
		const auto suffix = std::to_string(variant);
		const auto cell_name = "AnamCell_" + suffix;
		const auto translated = "KomorkaAnam_" + suffix;
		auto body = make_sub_record("INAM", std::string("info_01\0", 8)) + make_sub_record("ANAM", cell_name + '\0');
		return { tools_t::rec_type_t::cell, cell_name, cell_name, translated, make_record("INFO", body) };
	};

	const auto make_cndt_case = [](int variant) -> round_trip_case_t
	{
		const auto suffix = std::to_string(variant);
		const auto cell_name = "CndtCell_" + suffix;
		const auto translated = "KomorkaCndt_" + suffix;
		auto body = make_sub_record("NAME", std::string("npc_id\0", 7)) + make_sub_record("CNDT", cell_name + '\0');
		return { tools_t::rec_type_t::cell, cell_name, cell_name, translated, make_record("NPC_", body) };
	};

	constexpr int type_count = 12;
	const auto type_index = iteration % type_count;
	const auto variant = iteration;

	round_trip_case_t test_case;
	switch (type_index)
	{
	case 0:
		test_case = make_cell_case(variant);
		break;
	case 1:
		test_case = make_fnam_case(variant);
		break;
	case 2:
		test_case = make_dial_case(variant);
		break;
	case 3:
		test_case = make_gmst_case(variant);
		break;
	case 4:
		test_case = make_text_case(variant);
		break;
	case 5:
		test_case = make_desc_case(variant);
		break;
	case 6:
		test_case = make_rnam_case(variant);
		break;
	case 7:
		test_case = make_indx_case(variant);
		break;
	case 8:
		test_case = make_pgrd_case(variant);
		break;
	case 9:
		test_case = make_dnam_case(variant);
		break;
	case 10:
		test_case = make_anam_case(variant);
		break;
	case 11:
		test_case = make_cndt_case(variant);
		break;
	}

	auto merger = make_merger(
	    {
	        { test_case.record_type, test_case.key_text, test_case.old_text, test_case.new_text },
	    });

	auto esm_content = make_tes3_record() + test_case.esm_record;
	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find(test_case.new_text) != std::string::npos);
}

TEST_CASE("esm_converter_t, identity for unmatched records", "[u]")
{
	const auto iteration = GENERATE(range(0, 100));

	struct record_builder_t
	{
		std::string esm_content;
		size_t target_record_index;
	};

	const auto build_cell = [](int seed) -> record_builder_t
	{
		std::string cell_name = "UnmatchedCell_" + std::to_string(seed) + std::string(1, '\0');
		auto body = make_sub_record("NAME", cell_name);
		auto esm_content = make_tes3_record() + make_record("CELL", body);
		return { esm_content, 1 };
	};

	const auto build_pgrd = [](int seed) -> record_builder_t
	{
		std::string pgrd_name = "UnmatchedPgrd_" + std::to_string(seed) + std::string(1, '\0');
		auto body = make_sub_record("NAME", pgrd_name);
		auto esm_content = make_tes3_record() + make_record("PGRD", body);
		return { esm_content, 1 };
	};

	const auto build_fnam = [](int seed) -> record_builder_t
	{
		std::string npc_id = "npc_unmatched_" + std::to_string(seed) + std::string(1, '\0');
		std::string display_name = "Unmatched NPC " + std::to_string(seed) + std::string(1, '\0');
		auto body = make_sub_record("NAME", npc_id) + make_sub_record("FNAM", display_name);
		auto esm_content = make_tes3_record() + make_record("NPC_", body);
		return { esm_content, 1 };
	};

	const auto build_gmst = [](int seed) -> record_builder_t
	{
		std::string setting_name = "sUnmatched_" + std::to_string(seed) + std::string(1, '\0');
		std::string setting_value = "Value " + std::to_string(seed);
		auto body = make_sub_record("NAME", setting_name) + make_sub_record("STRV", setting_value);
		auto esm_content = make_tes3_record() + make_record("GMST", body);
		return { esm_content, 1 };
	};

	const auto build_desc = [](int seed) -> record_builder_t
	{
		std::string bsgn_name = "UnmatchedSign_" + std::to_string(seed) + std::string(1, '\0');
		std::string description = "Description text " + std::to_string(seed);
		auto body = make_sub_record("NAME", bsgn_name) + make_sub_record("DESC", description);
		auto esm_content = make_tes3_record() + make_record("BSGN", body);
		return { esm_content, 1 };
	};

	const auto build_text = [](int seed) -> record_builder_t
	{
		std::string book_id = "book_unmatched_" + std::to_string(seed) + std::string(1, '\0');
		std::string book_text = "Book content " + std::to_string(seed);
		auto body = make_sub_record("NAME", book_id) + make_sub_record("TEXT", book_text);
		auto esm_content = make_tes3_record() + make_record("BOOK", body);
		return { esm_content, 1 };
	};

	const auto build_rnam = [](int seed) -> record_builder_t
	{
		std::string faction_name = "UnmatchedFaction_" + std::to_string(seed) + std::string(1, '\0');
		std::string rank_name(32, '\0');
		std::string rank_text = "Rank" + std::to_string(seed);
		rank_text.copy(rank_name.data(), rank_text.size());
		auto body = make_sub_record("NAME", faction_name) + make_sub_record("RNAM", rank_name);
		auto esm_content = make_tes3_record() + make_record("FACT", body);
		return { esm_content, 1 };
	};

	const auto build_indx = [](int seed) -> record_builder_t
	{
		std::string indx_content(4, '\0');
		indx_content[0] = static_cast<char>(seed % 27);
		std::string desc_text = "Skill description " + std::to_string(seed);
		auto body = make_sub_record("INDX", indx_content) + make_sub_record("DESC", desc_text);
		auto esm_content = make_tes3_record() + make_record("SKIL", body);
		return { esm_content, 1 };
	};

	const auto build_dial = [](int seed) -> record_builder_t
	{
		std::string topic_name = "unmatched_topic_" + std::to_string(seed) + std::string(1, '\0');
		std::string topic_type(1, '\x00');
		auto body = make_sub_record("NAME", topic_name) + make_sub_record("DATA", topic_type);
		auto esm_content = make_tes3_record() + make_record("DIAL", body);
		return { esm_content, 1 };
	};

	const auto build_dnam = [](int seed) -> record_builder_t
	{
		std::string cell_id = "cell_id_" + std::to_string(seed) + std::string(1, '\0');
		std::string dest_name = "UnmatchedDest_" + std::to_string(seed) + std::string(1, '\0');
		auto body = make_sub_record("NAME", cell_id) + make_sub_record("DNAM", dest_name);
		auto esm_content = make_tes3_record() + make_record("CELL", body);
		return { esm_content, 1 };
	};

	const auto build_cndt = [](int seed) -> record_builder_t
	{
		std::string npc_id = "npc_cndt_" + std::to_string(seed) + std::string(1, '\0');
		std::string cell_name = "UnmatchedCndt_" + std::to_string(seed) + std::string(1, '\0');
		auto body = make_sub_record("NAME", npc_id) + make_sub_record("CNDT", cell_name);
		auto esm_content = make_tes3_record() + make_record("NPC_", body);
		return { esm_content, 1 };
	};

	constexpr int type_count = 11;
	const int type_index = iteration % type_count;

	record_builder_t built {};
	switch (type_index)
	{
	case 0:
		built = build_cell(iteration);
		break;
	case 1:
		built = build_pgrd(iteration);
		break;
	case 2:
		built = build_fnam(iteration);
		break;
	case 3:
		built = build_gmst(iteration);
		break;
	case 4:
		built = build_desc(iteration);
		break;
	case 5:
		built = build_text(iteration);
		break;
	case 6:
		built = build_rnam(iteration);
		break;
	case 7:
		built = build_indx(iteration);
		break;
	case 8:
		built = build_dial(iteration);
		break;
	case 9:
		built = build_dnam(iteration);
		break;
	case 10:
		built = build_cndt(iteration);
		break;
	}

	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "NeverMatchingKey_XYZ", "NeverMatchingText", "Translation" },
	        { tools_t::rec_type_t::fnam, "NPC_^never_matching_npc", "NeverMatching", "Nigdy" },
	        { tools_t::rec_type_t::dial, "never_matching_topic", "never_matching_topic", "nigdy_temat" },
	        { tools_t::rec_type_t::gmst, "sNeverMatching", "NeverMatch", "NigdyPasuje" },
	        { tools_t::rec_type_t::desc, "BSGN^NeverMatchSign", "NeverDesc", "NigdyOpis" },
	        { tools_t::rec_type_t::text, "never_match_book", "NeverText", "NigdyTekst" },
	        { tools_t::rec_type_t::rnam, "NeverFaction^0", "NeverRank", "NigdyRanga" },
	        { tools_t::rec_type_t::indx, "SKIL^099", "NeverSkill", "NigdyUmiejetnosc" },
	    });

	const auto esm_path = get_temp_path("yampt_test_identity_" + std::to_string(iteration) + ".esm");
	write_esm_file(built.esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	REQUIRE(records.size() > built.target_record_index);
	REQUIRE(records[built.target_record_index].modified == false);

	const auto & input_record_content =
	    built.esm_content.substr(built.esm_content.size() - records[built.target_record_index].content.size());
	REQUIRE(records[built.target_record_index].content == input_record_content);
}

TEST_CASE("esm_converter_t, CELL round-trip with random names", "[u]")
{
	const auto seed = GENERATE(range(0, 100));
	std::mt19937 rng(static_cast<unsigned>(seed * 7919 + 104729));
	std::uniform_int_distribution<int> len_dist(3, 30);
	std::uniform_int_distribution<int> char_dist(0, 51);

	const auto random_name = [&]() -> std::string
	{
		const int length = len_dist(rng);
		std::string name;
		name.reserve(length);
		for (int i = 0; i < length; ++i)
		{
			const int ch = char_dist(rng);
			if (ch < 26)
				name += static_cast<char>('A' + ch);
			else
				name += static_cast<char>('a' + ch - 26);
		}
		return name;
	};

	const auto cell_name = random_name();
	const auto translated = random_name() + "_PL";

	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, cell_name, cell_name, translated },
	    });

	auto body = make_sub_record("NAME", cell_name + '\0');
	auto esm_content = make_tes3_record() + make_record("CELL", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find(translated) != std::string::npos);
}

TEST_CASE("esm_converter_t, identity unmatched CELL random", "[u]")
{
	const auto seed = GENERATE(range(0, 100));
	std::mt19937 rng(static_cast<unsigned>(seed * 6991 + 83497));
	std::uniform_int_distribution<int> len_dist(4, 25);
	std::uniform_int_distribution<int> char_dist(0, 51);

	const auto random_name = [&](const std::string & prefix) -> std::string
	{
		const int length = len_dist(rng);
		std::string name = prefix;
		name.reserve(prefix.size() + length);
		for (int i = 0; i < length; ++i)
		{
			const int ch = char_dist(rng);
			if (ch < 26)
				name += static_cast<char>('A' + ch);
			else
				name += static_cast<char>('a' + ch - 26);
		}
		return name;
	};

	const auto esm_cell_name = random_name("Esm_");
	const auto dict_cell_name = random_name("Dict_");

	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, dict_cell_name, dict_cell_name, "Translated_" + dict_cell_name },
	    });

	auto body = make_sub_record("NAME", esm_cell_name + '\0');
	auto esm_content = make_tes3_record() + make_record("CELL", body);

	const auto esm_path = get_temp_path("yampt_test_id_cell_" + std::to_string(seed) + ".esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	REQUIRE(records.size() > 1);
	REQUIRE(records[1].modified == false);
}
