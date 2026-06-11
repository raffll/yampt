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

TEST_CASE("dict_writer serializes speaker fields when present", "[u][writer]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(tools_t::rec_type_t::info).insert({ "info_1", "Hello", "Cześć", "translated", "Fargoth", "M" });

	auto path = writer_test_dir + "/speaker_present.json";
	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	std::ifstream file(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	REQUIRE(content.find("Fargoth") != std::string::npos);
	REQUIRE(content.find("\"M\"") != std::string::npos);
}

TEST_CASE("dict_writer omits speaker fields when empty", "[u][writer]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(tools_t::rec_type_t::info).insert({ "info_no_spk", "Hello", "Cześć", "translated", "", "" });

	auto path = writer_test_dir + "/speaker_empty.json";
	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	std::ifstream file(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	REQUIRE(content.find("\"speaker_name\"") == std::string::npos);
	REQUIRE(content.find("\"gender\"") == std::string::npos);
}

TEST_CASE("dict_writer persists all status values", "[u][writer]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(tools_t::rec_type_t::cell).insert({ "k1", "o1", "n1", tools_t::status_t::missing });
	dict.at(tools_t::rec_type_t::cell).insert({ "k2", "o2", "n2", tools_t::status_t::coords });
	dict.at(tools_t::rec_type_t::cell).insert({ "k3", "o3", "n3", tools_t::status_t::reused });
	dict.at(tools_t::rec_type_t::cell).insert({ "k4", "o4", "n4", tools_t::status_t::error });

	auto path = writer_test_dir + "/all_statuses.json";
	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	std::ifstream file(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	REQUIRE(content.find("missing") != std::string::npos);
	REQUIRE(content.find("coords") != std::string::npos);
	REQUIRE(content.find("reused") != std::string::npos);
	REQUIRE(content.find("error") != std::string::npos);
}

TEST_CASE("dict_writer does not write encoding field", "[u][writer]")
{
	tools_t::dict_t dict = tools_t::initialize_dict();
	dict.at(tools_t::rec_type_t::cell).insert({ "Test", "Test", "Test", "untranslated" });

	auto path = writer_test_dir + "/no_encoding.json";
	dict_writer_t::write(dict, path);
	REQUIRE(fs::exists(path));

	std::ifstream file(path, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	REQUIRE(content.find("encoding") == std::string::npos);
}
