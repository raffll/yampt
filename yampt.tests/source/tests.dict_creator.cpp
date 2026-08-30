#include <catch2/catch_all.hpp>
#include <creator/cell_matcher.hpp>
#include <creator/dict_creator.hpp>
#include <utility/app_logger.hpp>
#include <cstring>
#include <filesystem>
#include <fstream>

TEST_CASE("dict_creator_t::differs_only_in_numbers_or_punct, identical strings", "[u]")
{
	REQUIRE(dict_creator_t::differs_only_in_numbers_or_punct("Balmora", "Balmora") == false);
}

TEST_CASE("dict_creator_t::differs_only_in_numbers_or_punct, different lengths", "[u]")
{
	REQUIRE(dict_creator_t::differs_only_in_numbers_or_punct("Balmora 1", "Balmora 12") == false);
}

TEST_CASE("dict_creator_t::differs_only_in_numbers_or_punct, number differs", "[u]")
{
	REQUIRE(dict_creator_t::differs_only_in_numbers_or_punct("Balmora 1", "Balmora 2") == true);
	REQUIRE(dict_creator_t::differs_only_in_numbers_or_punct("Room 001", "Room 002") == true);
	REQUIRE(dict_creator_t::differs_only_in_numbers_or_punct("Level 9", "Level 3") == true);
}

TEST_CASE("dict_creator_t::differs_only_in_numbers_or_punct, letter differs", "[u]")
{
	REQUIRE(dict_creator_t::differs_only_in_numbers_or_punct("Balmora", "Bolvora") == false);
	REQUIRE(dict_creator_t::differs_only_in_numbers_or_punct("Cell A", "Cell B") == false);
}

TEST_CASE("dict_creator_t::differs_only_in_numbers_or_punct, mixed differences", "[u]")
{
	REQUIRE(dict_creator_t::differs_only_in_numbers_or_punct("A1", "B2") == false);
}

TEST_CASE("dict_creator_t::adapt_translation, digit substitution", "[u]")
{
	auto result = dict_creator_t::adapt_translation("Room 2", "Room 1", "Pokoj 1");
	REQUIRE(result == "Pokoj 2");
}

TEST_CASE("dict_creator_t::adapt_translation, multiple digits", "[u]")
{
	auto result = dict_creator_t::adapt_translation("Level 35", "Level 12", "Poziom 12");
	REQUIRE(result == "Poziom 35");
}

TEST_CASE("dict_creator_t::adapt_translation, size mismatch unchanged", "[u]")
{
	auto result = dict_creator_t::adapt_translation("AB", "ABC", "XYZ");
	REQUIRE(result == "XYZ");
}

TEST_CASE("cell_matcher_t::is_interior_cell, interior flag set", "[u]")
{
	// DATA: flags byte 0 = 0x01 (interior), rest zeros
	std::string data(12, '\0');
	data[0] = '\x01';
	REQUIRE(cell_matcher_t::is_interior_cell(data) == true);
}

TEST_CASE("cell_matcher_t::is_interior_cell, exterior flag", "[u]")
{
	// DATA: flags byte 0 = 0x00 (exterior)
	std::string data(12, '\0');
	data[0] = '\x00';
	REQUIRE(cell_matcher_t::is_interior_cell(data) == false);
}

TEST_CASE("cell_matcher_t::is_interior_cell, interior with water", "[u]")
{
	// flags = 0x03 (interior + has water)
	std::string data(12, '\0');
	data[0] = '\x03';
	REQUIRE(cell_matcher_t::is_interior_cell(data) == true);
}

TEST_CASE("cell_matcher_t::is_interior_cell, too short data", "[u]")
{
	REQUIRE(cell_matcher_t::is_interior_cell("") == false);
	REQUIRE(cell_matcher_t::is_interior_cell("AB") == false);
}

TEST_CASE("cell_matcher_t::make_exterior_coord_key, positive coords", "[u]")
{
	// DATA: 4 bytes flags + 4 bytes GridX (3) + 4 bytes GridY (5)
	std::string data(12, '\0');
	int32_t x = 3, y = 5;
	std::memcpy(data.data() + 4, &x, 4);
	std::memcpy(data.data() + 8, &y, 4);
	REQUIRE(cell_matcher_t::make_exterior_coord_key(data) == "GRID[3,5]");
}

TEST_CASE("cell_matcher_t::make_exterior_coord_key, negative coords", "[u]")
{
	std::string data(12, '\0');
	int32_t x = -7, y = -12;
	std::memcpy(data.data() + 4, &x, 4);
	std::memcpy(data.data() + 8, &y, 4);
	REQUIRE(cell_matcher_t::make_exterior_coord_key(data) == "GRID[-7,-12]");
}

TEST_CASE("cell_matcher_t::make_exterior_coord_key, zero coords", "[u]")
{
	std::string data(12, '\0');
	REQUIRE(cell_matcher_t::make_exterior_coord_key(data) == "GRID[0,0]");
}

TEST_CASE("cell_matcher_t::make_exterior_coord_key, too short data", "[u]")
{
	REQUIRE(cell_matcher_t::make_exterior_coord_key("").empty());
	REQUIRE(cell_matcher_t::make_exterior_coord_key("12345678").empty());
}

TEST_CASE("cell_matcher_t::make_cell_key_text, produces 16-char hex string", "[u]")
{
	auto result = cell_matcher_t::make_cell_key_text("test fingerprint");
	REQUIRE(result.size() == 16);
	for (char c : result)
	{
		REQUIRE(((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')));
	}
}

TEST_CASE("cell_matcher_t::make_cell_key_text, same input same result", "[u]")
{
	auto a = cell_matcher_t::make_cell_key_text("Balmora fingerprint");
	auto b = cell_matcher_t::make_cell_key_text("Balmora fingerprint");
	REQUIRE(a == b);
}

TEST_CASE("cell_matcher_t::make_cell_key_text, different input produces different hash", "[u]")
{
	auto a = cell_matcher_t::make_cell_key_text("Balmora");
	auto b = cell_matcher_t::make_cell_key_text("Vivec");
	REQUIRE(a != b);
}

TEST_CASE("dict_creator_t::make_fnam, enchantment field populated for WEAP with ENAM", "[i]")
{
	const auto path = (std::filesystem::temp_directory_path() / "yampt_test_enchant_creator.esm").string();

	auto make_sub = [](const std::string & sub_id, const std::string & content) -> std::string
	{
		std::string result;
		result += sub_id;
		result += domain_types::convert_uint_to_string_byte_array(content.size());
		result += content;
		return result;
	};

	auto make_record = [&](const std::string & rec_id, const std::string & sub_records) -> std::string
	{
		std::string header;
		header += rec_id;
		header += domain_types::convert_uint_to_string_byte_array(sub_records.size());
		header += domain_types::convert_uint_to_string_byte_array(0);
		header += domain_types::convert_uint_to_string_byte_array(0);
		return header + sub_records;
	};

	std::string esm_content;
	esm_content += make_record("TES3", make_sub("HEDR", std::string(300, '\0')));

	const std::string weapon_name = std::string("iron_dagger") + '\0';
	const std::string weapon_fnam = std::string("Iron Dagger") + '\0';
	const std::string weapon_enam = std::string("fire_damage_en") + '\0';

	esm_content += make_record(
	    "WEAP", make_sub("NAME", weapon_name) + make_sub("FNAM", weapon_fnam) + make_sub("ENAM", weapon_enam));

	const std::string armor_name = std::string("steel_helm") + '\0';
	const std::string armor_fnam = std::string("Steel Helm") + '\0';

	esm_content += make_record("ARMO", make_sub("NAME", armor_name) + make_sub("FNAM", armor_fnam));

	{
		std::ofstream file(path, std::ios::binary);
		file.write(esm_content.data(), static_cast<std::streamsize>(esm_content.size()));
	}

	app_logger_t::reset_log();
	dict_creator_t creator(path);
	const auto & dict = creator.get_dict();

	auto fnam_it = dict.find(rec_type_t::fnam);
	REQUIRE(fnam_it != dict.end());

	const auto * dagger_entry = fnam_it->second.find("WEAP^iron_dagger");
	REQUIRE(dagger_entry != nullptr);
	REQUIRE(dagger_entry->enchantment == "fire_damage_en");

	const auto * helm_entry = fnam_it->second.find("ARMO^steel_helm");
	REQUIRE(helm_entry != nullptr);
	REQUIRE(helm_entry->enchantment.empty());

	std::filesystem::remove(path);
}
