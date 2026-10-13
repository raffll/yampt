#include <catch2/catch_all.hpp>
#include <io/binary_file_io.hpp>
#include <io/esm_reader.hpp>
#include <utility/app_logger.hpp>
#include <filesystem>

static std::string get_temp_path(const std::string & filename)
{
	return (std::filesystem::temp_directory_path() / filename).string();
}

static std::string make_sub_record(const std::string & sub_id, const std::string & data)
{
	std::string result;
	result += sub_id;
	result += domain_types::convert_uint_to_string_byte_array(data.size());
	result += data;
	return result;
}

static std::string make_record(const std::string & rec_id, const std::string & sub_records)
{
	std::string header;
	header += rec_id;
	header += domain_types::convert_uint_to_string_byte_array(sub_records.size());
	header += domain_types::convert_uint_to_string_byte_array(0);
	header += domain_types::convert_uint_to_string_byte_array(0);
	return header + sub_records;
}

[[maybe_unused]] static std::string make_esm_file(const std::vector<std::string> & records)
{
	std::string content;
	for (const auto & record : records)
		content += record;
	return content;
}

TEST_CASE("esm_reader_t::select_record, resets key and value", "[i]")
{
	auto tes3_body = make_sub_record("HEDR", std::string(300, '\0'));
	auto tes3 = make_record("TES3", tes3_body);

	auto gmst_body =
	    make_sub_record("NAME", std::string("sWelcome\0", 9)) + make_sub_record("STRV", std::string("Hello\0", 6));
	auto gmst = make_record("GMST", gmst_body);

	std::string file_content = tes3 + gmst;

	const auto temp_path = get_temp_path("yampt_test_select_record.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	REQUIRE(reader.is_loaded());

	reader.select_record(1);
	reader.set_key("NAME");
	REQUIRE(reader.get_key().exist == true);
	REQUIRE(reader.get_key().text == "sWelcome");

	reader.set_value("STRV");
	REQUIRE(reader.get_value().exist == true);
	REQUIRE(reader.get_value().text == "Hello");

	reader.select_record(0);
	REQUIRE(reader.get_key().exist == false);
	REQUIRE(reader.get_value().exist == false);
}

TEST_CASE("esm_reader_t::set_next_value, counter increments", "[i]")
{
	auto tes3_body = make_sub_record("HEDR", std::string(300, '\0'));
	auto tes3 = make_record("TES3", tes3_body);

	auto rnam1 = std::string("Rank One\0", 9);
	auto rnam2 = std::string("Rank Two\0", 9);
	auto rnam3 = std::string("Rank Three\0", 11);

	auto fact_body = make_sub_record("NAME", std::string("Fighters Guild\0", 15)) + make_sub_record("RNAM", rnam1) +
	                 make_sub_record("RNAM", rnam2) + make_sub_record("RNAM", rnam3);
	auto fact = make_record("FACT", fact_body);

	std::string file_content = tes3 + fact;

	const auto temp_path = get_temp_path("yampt_test_counter.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	REQUIRE(reader.is_loaded());

	reader.select_record(1);
	reader.set_value("RNAM");
	REQUIRE(reader.get_value().exist == true);
	REQUIRE(reader.get_value().text == "Rank One");
	REQUIRE(reader.get_value().counter == 0);

	reader.set_next_value("RNAM");
	REQUIRE(reader.get_value().exist == true);
	REQUIRE(reader.get_value().text == "Rank Two");
	REQUIRE(reader.get_value().counter == 1);

	reader.set_next_value("RNAM");
	REQUIRE(reader.get_value().exist == true);
	REQUIRE(reader.get_value().text == "Rank Three");
	REQUIRE(reader.get_value().counter == 2);

	reader.set_next_value("RNAM");
	REQUIRE(reader.get_value().exist == false);
}

TEST_CASE("esm_reader_t::set_key, independent of value state", "[i]")
{
	auto tes3_body = make_sub_record("HEDR", std::string(300, '\0'));
	auto tes3 = make_record("TES3", tes3_body);

	auto gmst_body =
	    make_sub_record("NAME", std::string("sSetting\0", 9)) + make_sub_record("STRV", std::string("Value\0", 6));
	auto gmst = make_record("GMST", gmst_body);

	std::string file_content = tes3 + gmst;

	const auto temp_path = get_temp_path("yampt_test_key_indep.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	REQUIRE(reader.is_loaded());

	reader.select_record(1);
	reader.set_value("STRV");
	REQUIRE(reader.get_value().exist == true);
	REQUIRE(reader.get_value().text == "Value");

	reader.set_key("NAME");
	REQUIRE(reader.get_key().exist == true);
	REQUIRE(reader.get_key().text == "sSetting");
	REQUIRE(reader.get_value().exist == true);
	REQUIRE(reader.get_value().text == "Value");
}

TEST_CASE("esm_reader_t::set_value, resets counter to zero", "[i]")
{
	auto tes3_body = make_sub_record("HEDR", std::string(300, '\0'));
	auto tes3 = make_record("TES3", tes3_body);

	auto rnam1 = std::string("First\0", 6);
	auto rnam2 = std::string("Second\0", 7);

	auto fact_body = make_sub_record("NAME", std::string("Guild\0", 6)) + make_sub_record("RNAM", rnam1) +
	                 make_sub_record("RNAM", rnam2);
	auto fact = make_record("FACT", fact_body);

	std::string file_content = tes3 + fact;

	const auto temp_path = get_temp_path("yampt_test_value_reset.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	reader.select_record(1);
	reader.set_value("RNAM");
	reader.set_next_value("RNAM");
	REQUIRE(reader.get_value().counter == 1);

	reader.set_value("RNAM");
	REQUIRE(reader.get_value().counter == 0);
	REQUIRE(reader.get_value().text == "First");
}

TEST_CASE("esm_reader_t::remove_record, erases record and shifts indices", "[i]")
{
	auto tes3 = make_record("TES3", make_sub_record("HEDR", std::string(300, '\0')));
	auto gmst_a = make_record("GMST", make_sub_record("NAME", std::string("sAlpha\0", 7)));
	auto gmst_b = make_record("GMST", make_sub_record("NAME", std::string("sBeta\0", 6)));
	auto gmst_c = make_record("GMST", make_sub_record("NAME", std::string("sGamma\0", 7)));

	std::string file_content = tes3 + gmst_a + gmst_b + gmst_c;

	const auto temp_path = get_temp_path("yampt_test_remove_record.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	REQUIRE(reader.is_loaded());
	REQUIRE(reader.get_records().size() == 4);

	reader.remove_record(2);

	REQUIRE(reader.get_records().size() == 3);
	REQUIRE(reader.get_records()[0].id == "TES3");
	REQUIRE(reader.get_records()[1].id == "GMST");
	REQUIRE(reader.get_records()[2].id == "GMST");

	reader.select_record(2);
	reader.set_key("NAME");
	REQUIRE(reader.get_key().exist == true);
	REQUIRE(reader.get_key().text == "sGamma");
}

TEST_CASE("esm_reader_t::split_file, parses consecutive records", "[i]")
{
	auto tes3 = make_record("TES3", make_sub_record("HEDR", std::string(300, '\0')));
	auto gmst = make_record("GMST", make_sub_record("NAME", std::string("sSetting\0", 9)));
	auto cell = make_record("CELL", make_sub_record("NAME", std::string("Balmora\0", 8)));
	auto npc = make_record("NPC_", make_sub_record("NAME", std::string("fargoth\0", 8)));

	std::string file_content = tes3 + gmst + cell + npc;

	const auto temp_path = get_temp_path("yampt_test_consecutive.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	REQUIRE(reader.is_loaded());
	REQUIRE(reader.get_records().size() == 4);
	REQUIRE(reader.get_records()[0].id == "TES3");
	REQUIRE(reader.get_records()[1].id == "GMST");
	REQUIRE(reader.get_records()[2].id == "CELL");
	REQUIRE(reader.get_records()[3].id == "NPC_");
}

TEST_CASE("esm_reader_t::split_file, stops on boundary drift", "[i]")
{
	auto tes3 = make_record("TES3", make_sub_record("HEDR", std::string(300, '\0')));
	auto gmst = make_record("GMST", make_sub_record("NAME", std::string("sSetting\0", 9)));

	auto cell_body = make_sub_record("NAME", std::string("Balmora\0", 8));
	std::string drift_header;
	drift_header += "CELL";
	drift_header += domain_types::convert_uint_to_string_byte_array(cell_body.size() + 2);
	drift_header += domain_types::convert_uint_to_string_byte_array(0);
	drift_header += domain_types::convert_uint_to_string_byte_array(0);
	auto drift_record = drift_header + cell_body;

	auto npc = make_record("NPC_", make_sub_record("NAME", std::string("fargoth\0", 8)));
	auto armo = make_record("ARMO", make_sub_record("NAME", std::string("iron_helm\0", 10)));

	std::string file_content = tes3 + gmst + drift_record + npc + armo;

	const auto temp_path = get_temp_path("yampt_test_drift.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	REQUIRE(reader.get_records().size() == 2);
	REQUIRE(reader.get_records()[0].id == "TES3");
	REQUIRE(reader.get_records()[1].id == "GMST");

	for (const auto & record : reader.get_records())
	{
		REQUIRE(record.id != "CELL");
		REQUIRE(record.id != "NPC_");
		REQUIRE(record.id != "ARMO");
	}
}

TEST_CASE("esm_reader_t::split_file, valid records with sub-records tile and parse", "[i]")
{
	auto tes3 = make_record("TES3", make_sub_record("HEDR", std::string(300, '\0')));

	auto npc_body = make_sub_record("NAME", std::string("fargoth\0", 8)) +
	                make_sub_record("FNAM", std::string("Fargoth\0", 8)) +
	                make_sub_record("RNAM", std::string("Wood Elf\0", 9));
	auto npc = make_record("NPC_", npc_body);

	auto info = make_record("INFO", "");

	std::string file_content = tes3 + npc + info;

	const auto temp_path = get_temp_path("yampt_test_tile_valid.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	REQUIRE(reader.is_loaded());
	REQUIRE(reader.get_records().size() == 3);
	REQUIRE(reader.get_records()[0].id == "TES3");
	REQUIRE(reader.get_records()[1].id == "NPC_");
	REQUIRE(reader.get_records()[2].id == "INFO");
}

TEST_CASE("esm_reader_t::split_file, stops on sub-record tiling mismatch", "[i]")
{
	auto tes3 = make_record("TES3", make_sub_record("HEDR", std::string(300, '\0')));

	auto cell_body =
	    make_sub_record("NAME", std::string("Balmora\0", 8)) + make_sub_record("FRMR", std::string(4, '\0'));
	std::string mis_sized_header;
	mis_sized_header += "CELL";
	mis_sized_header += domain_types::convert_uint_to_string_byte_array(cell_body.size() - 8);
	mis_sized_header += domain_types::convert_uint_to_string_byte_array(0);
	mis_sized_header += domain_types::convert_uint_to_string_byte_array(0);
	auto mis_sized_record = mis_sized_header + cell_body;

	auto npc = make_record("NPC_", make_sub_record("NAME", std::string("fargoth\0", 8)));
	auto armo = make_record("ARMO", make_sub_record("NAME", std::string("iron_helm\0", 10)));

	std::string file_content = tes3 + mis_sized_record + npc + armo;

	const auto temp_path = get_temp_path("yampt_test_tile_mismatch.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	REQUIRE(reader.get_records().size() == 1);
	REQUIRE(reader.get_records()[0].id == "TES3");

	for (const auto & record : reader.get_records())
	{
		REQUIRE(record.id != "NPC_");
		REQUIRE(record.id != "ARMO");
	}
}

TEST_CASE("esm_reader_t::split_file, empty-body record tiles", "[i]")
{
	auto tes3 = make_record("TES3", make_sub_record("HEDR", std::string(300, '\0')));
	auto dial = make_record("DIAL", "");

	std::string file_content = tes3 + dial;

	const auto temp_path = get_temp_path("yampt_test_tile_empty.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	REQUIRE(reader.is_loaded());
	REQUIRE(reader.get_records().size() == 2);
	REQUIRE(reader.get_records()[0].id == "TES3");
	REQUIRE(reader.get_records()[1].id == "DIAL");
}
