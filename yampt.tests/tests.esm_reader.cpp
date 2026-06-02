#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/esm_reader.hpp"

#include <string>

extern std::string g_master_path;

using namespace std;

TEST_CASE("loading and parsing plugin file", "[i]")
{
	esm_reader_t esm((g_master_path + "en/Morrowind.esm"));
	REQUIRE(esm.is_loaded() == true);
	REQUIRE(esm.get_name().full == "Morrowind.esm");
	REQUIRE(esm.get_name().name == "Morrowind");
	REQUIRE(esm.get_name().ext == ".esm");
}

TEST_CASE("set key, not found", "[i]")
{
	esm_reader_t esm((g_master_path + "en/Morrowind.esm"));
	esm.select_record(0);
	esm.set_key("TEST");
	REQUIRE(esm.get_key().id == "TEST");
	REQUIRE(esm.get_key().exist == false);
	REQUIRE(esm.get_key().text == "N/A");
}

TEST_CASE("set key, empty text", "[i]")
{
	esm_reader_t esm((g_master_path + "en/Morrowind.esm"));
	esm.select_record(16955); /* region CELL with empty NAME */
	esm.set_key("NAME");
	REQUIRE(esm.get_key().id == "NAME");
	REQUIRE(esm.get_key().exist == true);
	REQUIRE(esm.get_key().text == "");
}

TEST_CASE("set key", "[i]")
{
	esm_reader_t esm((g_master_path + "en/Morrowind.esm"));
	esm.select_record(22075); /* regular CELL */
	esm.set_key("NAME");
	REQUIRE(esm.get_key().id == "NAME");
	REQUIRE(esm.get_key().exist == true);
	REQUIRE(esm.get_key().text == "Zergonipal, Shrine");
}

TEST_CASE("set key, INDX case", "[i]")
{
	esm_reader_t esm((g_master_path + "en/Morrowind.esm"));
	esm.select_record(2062); /* SKIL with INDX instead of NAME */
	esm.set_key("INDX");
	REQUIRE(esm.get_key().id == "INDX");
	REQUIRE(esm.get_key().exist == true);
	REQUIRE(tools_t::get_indx(esm.get_key().content) == "000");
}

TEST_CASE("set key, dialog type case", "[i]")
{
	esm_reader_t esm((g_master_path + "en/Morrowind.esm"));
	esm.select_record(22245); /* DIAL with one byte DATA dialog type */
	esm.set_key("DATA");
	REQUIRE(esm.get_key().id == "DATA");
	REQUIRE(esm.get_key().exist == true);
	REQUIRE(tools_t::get_dialog_type(esm.get_key().content) == "J");
}

TEST_CASE("set value, not found", "[i]")
{
	esm_reader_t esm((g_master_path + "en/Morrowind.esm"));
	esm.select_record(0);
	esm.set_value("TEST");
	REQUIRE(esm.get_value().id == "TEST");
	REQUIRE(esm.get_value().exist == false);
	REQUIRE(esm.get_value().text == "N/A");
	REQUIRE(esm.get_value().pos == esm.get_record().content.size());
	REQUIRE(esm.get_value().size == 0);
}

TEST_CASE("set value", "[i]")
{
	/*
	   (16) FACT............
	   (16) NAME....Redoran.
	   (28) FNAM....Great House Redoran.
   (pos) -> RNAM....Hireling........................
	                ^            size              ^
	*/

	esm_reader_t esm((g_master_path + "en/Morrowind.esm"));
	esm.select_record(1600); /* RNAM */
	esm.set_value("RNAM");
	REQUIRE(esm.get_value().id == "RNAM");
	REQUIRE(esm.get_value().exist == true);
	REQUIRE(esm.get_value().text == "Hireling");
	REQUIRE(esm.get_value().content.size() == 32);
	REQUIRE(esm.get_value().pos == (16 + 16 + 28));
	REQUIRE(esm.get_value().size == 32);
}

TEST_CASE("set value, next", "[i]")
{
	/*
	   (16) FACT............
	   (16) NAME....Redoran.
	   (28) FNAM....Great House Redoran.
	   (40) RNAM....Hireling........................
   (pos) -> RNAM....Retainer........................
	                ^            size              ^
	*/

	esm_reader_t esm((g_master_path + "en/Morrowind.esm"));
	esm.select_record(1600); /* RNAM */
	esm.set_value("RNAM");
	esm.set_next_value("RNAM");
	REQUIRE(esm.get_value().id == "RNAM");
	REQUIRE(esm.get_value().exist == true);
	REQUIRE(esm.get_value().content.size() == 32);
	REQUIRE(esm.get_value().text == "Retainer");
	REQUIRE(esm.get_value().pos == (16 + 16 + 28 + 40));
	REQUIRE(esm.get_value().size == 32);
}

TEST_CASE("set_modified increments count", "[i]")
{
	esm_reader_t esm((g_master_path + "en/Morrowind.esm"));
	REQUIRE(esm.is_loaded() == true);
	size_t before = esm.get_modified_count();
	esm.set_modified(0);
	REQUIRE(esm.get_modified_count() == before + 1);
}

TEST_CASE("loading non-existent file", "[i]")
{
	esm_reader_t esm("this/path/does/not/exist.esm");
	REQUIRE(esm.is_loaded() == false);
}

TEST_CASE("loading non-TES3 file", "[i]")
{
	const std::string temp_path = "temp_not_tes3.esm";
	{
		std::ofstream f(temp_path, std::ios::binary);
		f << "XXXX\x00\x00\x00\x00not a tes3 file";
	}
	esm_reader_t esm(temp_path);
	REQUIRE(esm.is_loaded() == false);
	std::remove(temp_path.c_str());
}

TEST_CASE("split_file bounds check on corrupt record size", "[u]")
{
	const std::string temp_path = "temp_corrupt_size.esm";
	{
		std::ofstream f(temp_path, std::ios::binary);
		// Valid TES3 header record: "TES3" + size (4 bytes LE) + 8 padding bytes + 4 bytes data
		// Record size field = total_record_size - 16, so for a 20-byte record: size field = 4
		std::string header = "TES3";
		header += std::string("\x04\x00\x00\x00", 4); // size = 4 (record is 20 bytes total)
		header += std::string(8, '\0');               // 8 bytes flags/padding
		header += std::string(4, '\0');               // 4 bytes of data
		// Second record with garbage size that exceeds file
		std::string corrupt = "GLOB";
		corrupt += std::string("\xFF\xFF\x00\x00", 4); // size = 65535, way beyond file
		corrupt += std::string(8, '\0');               // 8 bytes flags/padding
		f << header << corrupt;
	}
	esm_reader_t esm(temp_path);
	REQUIRE(esm.is_loaded() == true);
	REQUIRE(esm.get_records().size() == 1); // only the first valid record retained
	std::remove(temp_path.c_str());
}
