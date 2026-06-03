#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/dict_writer.hpp"
#include "../yampt/dict_reader.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static const std::string writer_test_dir = "tests/writer";

TEST_CASE("dict_writer setup", "[u][writer]")
{
	fs::create_directories(writer_test_dir);
	REQUIRE(fs::is_directory(writer_test_dir));
}

TEST_CASE("dict_writer writes encoding metadata", "[u][writer]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(tools_t::rec_type_t::cell).insert({ "Test", "Test", "Test", "untranslated" });

	auto path = writer_test_dir + "/encoding_1250.json";
	dict_writer_t::write(dict, path, tools_t::encoding_t::windows_1250);
	REQUIRE(fs::exists(path));
	REQUIRE(fs::file_size(path) > 0);

	std::ifstream file(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	REQUIRE(content.find("windows-1250") != std::string::npos);
}

TEST_CASE("dict_writer writes encoding metadata windows-1251", "[u][writer]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(tools_t::rec_type_t::cell).insert({ "Test", "Test", "Test", "untranslated" });

	auto path = writer_test_dir + "/encoding_1251.json";
	dict_writer_t::write(dict, path, tools_t::encoding_t::windows_1251);
	REQUIRE(fs::exists(path));

	std::ifstream file(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	REQUIRE(content.find("windows-1251") != std::string::npos);
}

TEST_CASE("dict_writer serializes speaker fields when present", "[u][writer]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(tools_t::rec_type_t::info).insert(
	    { "info_1", "Hello", "Cześć", "translated", "fargoth", "Fargoth", "M" });

	auto path = writer_test_dir + "/speaker_present.json";
	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	std::ifstream file(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	REQUIRE(content.find("fargoth") != std::string::npos);
	REQUIRE(content.find("Fargoth") != std::string::npos);
	REQUIRE(content.find("\"M\"") != std::string::npos);
}

TEST_CASE("dict_writer omits speaker fields when empty", "[u][writer]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(tools_t::rec_type_t::info).insert(
	    { "info_no_spk", "Hello", "Cześć", "translated", "", "", "" });

	auto path = writer_test_dir + "/speaker_empty.json";
	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	std::ifstream file(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	REQUIRE(content.find("\"speaker\"") == std::string::npos);
	REQUIRE(content.find("\"speaker_name\"") == std::string::npos);
	REQUIRE(content.find("\"gender\"") == std::string::npos);
}

TEST_CASE("dict_writer persists all status values", "[u][writer]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(tools_t::rec_type_t::cell).insert({ "k1", "o1", "n1", tools_t::status_t::missing });
	dict.at(tools_t::rec_type_t::cell).insert({ "k2", "o2", "n2", tools_t::status_t::matched_by_coords });
	dict.at(tools_t::rec_type_t::cell).insert({ "k3", "o3", "n3", tools_t::status_t::auto_translated });
	dict.at(tools_t::rec_type_t::cell).insert({ "k4", "o4", "n4", tools_t::status_t::has_errors });

	auto path = writer_test_dir + "/all_statuses.json";
	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	std::ifstream file(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	REQUIRE(content.find("missing") != std::string::npos);
	REQUIRE(content.find("matched_by_coords") != std::string::npos);
	REQUIRE(content.find("auto_translated") != std::string::npos);
	REQUIRE(content.find("has_errors") != std::string::npos);
}

TEST_CASE("dict_writer with default encoding uses windows-1252", "[u][writer]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(tools_t::rec_type_t::cell).insert({ "Test", "Test", "Test", "untranslated" });

	auto path = writer_test_dir + "/default_encoding.json";
	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	std::ifstream file(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	REQUIRE(content.find("windows-1252") != std::string::npos);
}
