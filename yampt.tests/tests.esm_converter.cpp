#include <catch2/catch_all.hpp>
#include "../yampt/model/esm_converter.hpp"
#include "../yampt/io/dict_writer.hpp"

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

static dict_merger_t make_merger_with_entry(
    tools_t::rec_type_t record_type,
    const std::string & key_text,
    const std::string & old_text,
    const std::string & new_text)
{
	const auto dict_path = get_temp_path("yampt_test_converter_dict.json");
	tools_t::reset_log();

	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(record_type).insert({ key_text, old_text, new_text, "translated" });
	dict_writer_t::write(dict, dict_path);

	tools_t::reset_log();
	dict_merger_t merger({ dict_path });
	tools_t::reset_log();
	std::filesystem::remove(dict_path);
	return merger;
}

static dict_merger_t make_merger_with_entries(
    const std::vector<std::tuple<tools_t::rec_type_t, std::string, std::string, std::string>> & entries)
{
	const auto dict_path = get_temp_path("yampt_test_converter_dict.json");
	tools_t::reset_log();

	tools_t::dict_t dict = tools_t::initialize_dict();
	for (const auto & [record_type, key_text, old_text, new_text] : entries)
		dict.at(record_type).insert({ key_text, old_text, new_text, "translated" });

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

TEST_CASE("esm_converter_t::convert_cell, basic conversion", "[i]")
{
	auto merger = make_merger_with_entry(tools_t::rec_type_t::cell, "Balmora", "Balmora", "Balmora PL");

	auto cell_body = make_sub_record("NAME", std::string("Balmora\0", 8));
	auto esm_content = make_tes3_record() + make_record("CELL", cell_body);

	const auto esm_path = get_temp_path("yampt_test_conv_cell.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	REQUIRE(records.size() == 2);

	const auto & cell_content = records[1].content;
	REQUIRE(cell_content.find("Balmora PL") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_cell, empty name is skipped", "[i]")
{
	auto merger = make_merger_with_entry(tools_t::rec_type_t::cell, "", "", "Something");

	auto cell_body = make_sub_record("NAME", std::string("\0", 1));
	auto esm_content = make_tes3_record() + make_record("CELL", cell_body);

	const auto esm_path = get_temp_path("yampt_test_conv_cell_empty.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	const auto & cell_content = records[1].content;
	REQUIRE(cell_content.find("Something") == std::string::npos);
}

TEST_CASE("esm_converter_t::convert_fnam, basic conversion", "[i]")
{
	auto merger = make_merger_with_entry(tools_t::rec_type_t::fnam, "NPC_^fargoth", "Fargoth", "Fargoth PL");

	auto npc_body =
	    make_sub_record("NAME", std::string("fargoth\0", 8)) + make_sub_record("FNAM", std::string("Fargoth\0", 8));
	auto esm_content = make_tes3_record() + make_record("NPC_", npc_body);

	const auto esm_path = get_temp_path("yampt_test_conv_fnam.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	const auto & npc_content = records[1].content;
	REQUIRE(npc_content.find("Fargoth PL") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_fnam, player record is skipped", "[i]")
{
	auto merger = make_merger_with_entry(tools_t::rec_type_t::fnam, "NPC_^player", "Player", "Gracz");

	auto npc_body =
	    make_sub_record("NAME", std::string("player\0", 7)) + make_sub_record("FNAM", std::string("Player\0", 7));
	auto esm_content = make_tes3_record() + make_record("NPC_", npc_body);

	const auto esm_path = get_temp_path("yampt_test_conv_fnam_player.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	const auto & npc_content = records[1].content;
	REQUIRE(npc_content.find("Gracz") == std::string::npos);
}

TEST_CASE("esm_converter_t::convert_gmst, string setting converted", "[i]")
{
	auto merger = make_merger_with_entry(tools_t::rec_type_t::gmst, "sWelcome", "Welcome", "Witaj");

	auto gmst_body =
	    make_sub_record("NAME", std::string("sWelcome\0", 9)) + make_sub_record("STRV", std::string("Welcome", 7));
	auto esm_content = make_tes3_record() + make_record("GMST", gmst_body);

	const auto esm_path = get_temp_path("yampt_test_conv_gmst.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	const auto & gmst_content = records[1].content;
	REQUIRE(gmst_content.find("Witaj") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_gmst, non-string setting is skipped", "[i]")
{
	auto merger = make_merger_with_entry(tools_t::rec_type_t::gmst, "iMaxLevel", "100", "200");

	std::string int_value(4, '\0');
	int_value[0] = 100;
	auto gmst_body = make_sub_record("NAME", std::string("iMaxLevel\0", 10)) + make_sub_record("INTV", int_value);
	auto esm_content = make_tes3_record() + make_record("GMST", gmst_body);

	const auto esm_path = get_temp_path("yampt_test_conv_gmst_int.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_dial, topic type converted", "[i]")
{
	auto merger = make_merger_with_entry(tools_t::rec_type_t::dial, "background", "background", "tlo");

	std::string dial_type(1, '\0');
	auto dial_body = make_sub_record("NAME", std::string("background\0", 11)) + make_sub_record("DATA", dial_type);
	auto esm_content = make_tes3_record() + make_record("DIAL", dial_body);

	const auto esm_path = get_temp_path("yampt_test_conv_dial.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	const auto & dial_content = records[1].content;
	REQUIRE(dial_content.find("tlo") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_dial, journal type is skipped", "[i]")
{
	auto merger = make_merger_with_entry(tools_t::rec_type_t::dial, "quest_entry", "quest_entry", "wpis_zadania");

	std::string journal_type(1, '\x04');
	auto dial_body = make_sub_record("NAME", std::string("quest_entry\0", 12)) + make_sub_record("DATA", journal_type);
	auto esm_content = make_tes3_record() + make_record("DIAL", dial_body);

	const auto esm_path = get_temp_path("yampt_test_conv_dial_journal.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	REQUIRE(records[1].modified == false);
}

TEST_CASE("esm_converter_t::convert_info, preceding DIAL sets key prefix", "[i]")
{
	auto merger = make_merger_with_entry(tools_t::rec_type_t::info, "T^background^info_001", "Hello there", "Witaj");

	std::string topic_type(1, '\0');
	auto dial_body = make_sub_record("NAME", std::string("background\0", 11)) + make_sub_record("DATA", topic_type);
	auto dial_record = make_record("DIAL", dial_body);

	auto info_body =
	    make_sub_record("INAM", std::string("info_001\0", 9)) + make_sub_record("NAME", std::string("Hello there", 11));
	auto info_record = make_record("INFO", info_body);

	auto esm_content = make_tes3_record() + dial_record + info_record;

	const auto esm_path = get_temp_path("yampt_test_conv_info.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	const auto & info_content = records[2].content;
	REQUIRE(info_content.find("Witaj") != std::string::npos);
}

TEST_CASE("esm_converter_t::convert_info, marks DIAL as modified", "[i]")
{
	auto merger = make_merger_with_entry(tools_t::rec_type_t::info, "T^greetings^resp_42", "Good day", "Dzien dobry");

	std::string topic_type(1, '\0');
	auto dial_body = make_sub_record("NAME", std::string("greetings\0", 10)) + make_sub_record("DATA", topic_type);
	auto dial_record = make_record("DIAL", dial_body);

	auto info_body =
	    make_sub_record("INAM", std::string("resp_42\0", 8)) + make_sub_record("NAME", std::string("Good day", 8));
	auto info_record = make_record("INFO", info_body);

	auto esm_content = make_tes3_record() + dial_record + info_record;

	const auto esm_path = get_temp_path("yampt_test_conv_info_dial_mod.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	REQUIRE(records[1].modified == true);
}

TEST_CASE("esm_converter_t::convert_scpt, SCTX and SCDT patched", "[i]")
{
	auto merger = make_merger_with_entries(
	    {
	        { tools_t::rec_type_t::cell, "Balmora", "Balmora", "Balmora PL" },
	    });

	std::string old_sctx = "GetPCCell \"Balmora\"";

	std::string old_scdt;
	old_scdt += std::string(10, '\x00');
	std::string size_byte(1, static_cast<char>(7));
	old_scdt += size_byte;
	old_scdt += "Balmora";
	old_scdt += std::string(5, '\x00');

	std::string schd_content(48, '\0');
	std::string script_name = "TestScript";
	schd_content.replace(0, script_name.size(), script_name);
	schd_content.erase(44, 4);
	schd_content.insert(44, tools_t::convert_uint_to_string_byte_array(old_scdt.size()));

	auto scpt_body =
	    make_sub_record("SCHD", schd_content) + make_sub_record("SCDT", old_scdt) + make_sub_record("SCTX", old_sctx);
	auto esm_content = make_tes3_record() + make_record("SCPT", scpt_body);

	const auto esm_path = get_temp_path("yampt_test_conv_scpt.esm");
	write_esm_file(esm_content, esm_path);
	tools_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & records = converter.get_records();
	const auto & scpt_content = records[1].content;
	REQUIRE(scpt_content.find("Balmora PL") != std::string::npos);
	REQUIRE(records[1].modified == true);
}
