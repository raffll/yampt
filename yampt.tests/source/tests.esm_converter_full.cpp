#include <catch2/catch_all.hpp>
#include <io/dict_writer.hpp>
#include <model/esm_converter.hpp>

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

TEST_CASE("esm_converter_t::convert_desc, BSGN converted", "[i]")
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

TEST_CASE("esm_converter_t::convert_desc, CLAS converted", "[i]")
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

TEST_CASE("esm_converter_t::convert_desc, RACE converted", "[i]")
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

TEST_CASE("esm_converter_t::convert_desc, non-desc record type is skipped", "[i]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::desc, "NPC_^someone", "text", "tekst" },
	    });

	auto body = make_sub_record("NAME", std::string("someone\0", 8)) + make_sub_record("DESC", std::string("text", 4));
	auto esm_content = make_tes3_record() + make_record("NPC_", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("tekst") == std::string::npos);
}

TEST_CASE("esm_converter_t::convert_text, BOOK converted", "[i]")
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

TEST_CASE("esm_converter_t::convert_text, non-BOOK is skipped", "[i]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::text, "item_001", "text", "tekst" },
	    });

	auto body = make_sub_record("NAME", std::string("item_001\0", 9)) + make_sub_record("TEXT", std::string("text", 4));
	auto esm_content = make_tes3_record() + make_record("MISC", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("tekst") == std::string::npos);
}

TEST_CASE("esm_converter_t::convert_rnam, FACT rank converted", "[i]")
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

TEST_CASE("esm_converter_t::convert_indx, SKIL converted", "[i]")
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

TEST_CASE("esm_converter_t::convert_indx, MGEF converted", "[i]")
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

TEST_CASE("esm_converter_t::convert_pgrd, cell name converted", "[i]")
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

TEST_CASE("esm_converter_t::convert_pgrd, empty name is skipped", "[i]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "", "", "Something" },
	    });

	auto body = make_sub_record("NAME", std::string("\0", 1));
	auto esm_content = make_tes3_record() + make_record("PGRD", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Something") == std::string::npos);
}

TEST_CASE("esm_converter_t::convert_anam, INFO cell name converted", "[i]")
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

TEST_CASE("esm_converter_t::convert_anam, empty ANAM is skipped", "[i]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "", "", "Something" },
	    });

	auto body = make_sub_record("INAM", std::string("info_01\0", 8)) + make_sub_record("ANAM", std::string("\0", 1));
	auto esm_content = make_tes3_record() + make_record("INFO", body);

	const auto & result = run_converter(esm_content, merger);
	REQUIRE(result.find("Something") == std::string::npos);
}

TEST_CASE("esm_converter_t::convert_dnam, door destination converted", "[i]")
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

TEST_CASE("esm_converter_t::convert_cndt, NPC escort cell converted", "[i]")
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

TEST_CASE("esm_converter_t::convert_bnam, INFO result script converted", "[i]")
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

	const auto esm_path = get_temp_path("yampt_test_full_conv.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	const auto & info_content = records[2].content;
	REQUIRE(info_content.find("AddTopic \"tlo\"") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_bnam, INFO without BNAM is skipped", "[i]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::dial, "topic", "topic", "temat" },
	    });

	std::string topic_type(1, '\0');
	auto dial_body = make_sub_record("NAME", std::string("topic\0", 6)) + make_sub_record("DATA", topic_type);
	auto dial_record = make_record("DIAL", dial_body);

	auto info_body = make_sub_record("INAM", std::string("info_01\0", 8)) +
	                 make_sub_record("NAME", std::string("response text", 13));
	auto info_record = make_record("INFO", info_body);

	auto esm_content = make_tes3_record() + dial_record + info_record;

	const auto esm_path = get_temp_path("yampt_test_full_conv.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());
	REQUIRE(converter.get_records()[2].modified == false);
}

TEST_CASE("esm_converter_t::convert_scpt, identical script is not modified", "[i]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Balmora", "Balmora", "Balmora PL" },
	    });

	std::string old_sctx = "set x to 1";

	std::string old_scdt(16, '\x00');

	std::string schd_content(48, '\0');
	std::string script_name = "NoMatchScript";
	schd_content.replace(0, script_name.size(), script_name);
	schd_content.erase(44, 4);
	schd_content.insert(44, tools_t::convert_uint_to_string_byte_array(old_scdt.size()));

	auto scpt_body =
	    make_sub_record("SCHD", schd_content) + make_sub_record("SCDT", old_scdt) + make_sub_record("SCTX", old_sctx);
	auto esm_content = make_tes3_record() + make_record("SCPT", scpt_body);

	const auto esm_path = get_temp_path("yampt_test_full_conv.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());
	REQUIRE(converter.get_records()[1].modified == false);
}

TEST_CASE("esm_converter_t, only translated status is applied", "[i]")
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

TEST_CASE("esm_converter_t, translated status is applied", "[i]")
{
	const auto dict_path = get_temp_path("yampt_test_full_conv_dict.json");
	tools_t::reset_log();

	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(tools_t::rec_type_t::cell).insert({ "Balmora", "Balmora", "Balmora PL", status_t::translated });
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

	const auto & records = converter.get_records();
	const auto & cell_content = records[1].content;
	REQUIRE(cell_content.find("Balmora PL") != std::string::npos);
}

TEST_CASE("esm_converter_t, missing dict entry leaves record unchanged", "[i]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Ald-ruhn", "Ald-ruhn", "Ald-ruhn PL" },
	    });

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

TEST_CASE("esm_converter_t, identical old and new text is not applied", "[i]")
{
	auto merger = make_merger(
	    {
	        { tools_t::rec_type_t::cell, "Balmora", "Balmora", "Balmora" },
	    });

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
