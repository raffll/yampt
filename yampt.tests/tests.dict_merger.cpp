#include "catch.hpp"
#include "../yampt/tools.hpp"
#include "../yampt/dict_merger.hpp"

#include <fstream>
#include <cstdio>

static void write_temp_dict(const std::string & path, const std::string & content)
{
	std::ofstream f(path, std::ios::binary);
	f << content;
}

static void remove_temp_file(const std::string & path)
{
	std::remove(path.c_str());
}

TEST_CASE("dict_merger_t add_record inserts entry", "[u]")
{
	dict_merger_t merger;
	merger.add_record(tools_t::rec_type_t::cell, "Balmora", "BalmoraTrans");
	const auto & dict = merger.get_dict();
	const auto * entry = dict.at(tools_t::rec_type_t::cell).find("Balmora");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->new_text == "BalmoraTrans");
}

TEST_CASE("dict_merger_t merge first-wins precedence", "[i]")
{
	const std::string path1 = "temp_merger_first.json";
	const std::string path2 = "temp_merger_second.json";

	tools_t::reset_log();

	write_temp_dict(
	    path1,
	    "{\n"
	    "  \"CELL\": [\n"
	    "    { \"id\": \"Balmora\", \"old\": \"\", \"new\": \"FirstValue\", \"status\": \"translated\" }\n"
	    "  ]\n"
	    "}\n");

	write_temp_dict(
	    path2,
	    "{\n"
	    "  \"CELL\": [\n"
	    "    { \"id\": \"Balmora\", \"old\": \"\", \"new\": \"SecondValue\", \"status\": \"translated\" }\n"
	    "  ]\n"
	    "}\n");

	dict_merger_t merger({ path1, path2 });
	const auto * entry = merger.get_dict().at(tools_t::rec_type_t::cell).find("Balmora");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->new_text == "FirstValue");

	remove_temp_file(path1);
	remove_temp_file(path2);
}

TEST_CASE("dict_merger_t key only in second dict", "[i]")
{
	const std::string path1 = "temp_merger_second_only1.json";
	const std::string path2 = "temp_merger_second_only2.json";

	tools_t::reset_log();

	write_temp_dict(
	    path1,
	    "{\n"
	    "  \"CELL\": [\n"
	    "    { \"id\": \"Vivec\", \"old\": \"\", \"new\": \"VivecTrans\", \"status\": \"translated\" }\n"
	    "  ]\n"
	    "}\n");

	write_temp_dict(
	    path2,
	    "{\n"
	    "  \"CELL\": [\n"
	    "    { \"id\": \"Balmora\", \"old\": \"\", \"new\": \"BalmoraTrans\", \"status\": \"translated\" }\n"
	    "  ]\n"
	    "}\n");

	dict_merger_t merger({ path1, path2 });
	const auto * entry = merger.get_dict().at(tools_t::rec_type_t::cell).find("Balmora");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->new_text == "BalmoraTrans");

	remove_temp_file(path1);
	remove_temp_file(path2);
}

TEST_CASE("dict_merger_t identical values", "[i]")
{
	const std::string path1 = "temp_merger_ident1.json";
	const std::string path2 = "temp_merger_ident2.json";

	tools_t::reset_log();

	write_temp_dict(
	    path1,
	    "{\n"
	    "  \"CELL\": [\n"
	    "    { \"id\": \"Balmora\", \"old\": \"\", \"new\": \"Balmora\", \"status\": \"translated\" }\n"
	    "  ]\n"
	    "}\n");

	write_temp_dict(
	    path2,
	    "{\n"
	    "  \"CELL\": [\n"
	    "    { \"id\": \"Balmora\", \"old\": \"\", \"new\": \"Balmora\", \"status\": \"translated\" }\n"
	    "  ]\n"
	    "}\n");

	dict_merger_t merger({ path1, path2 });
	const auto & chapter = merger.get_dict().at(tools_t::rec_type_t::cell);
	const auto * entry = chapter.find("Balmora");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->new_text == "Balmora");
	REQUIRE(chapter.size() == 1);

	remove_temp_file(path1);
	remove_temp_file(path2);
}

TEST_CASE("dict_merger_t duplicate values warning", "[i]")
{
	const std::string path1 = "temp_merger_dupval.json";

	tools_t::reset_log();

	write_temp_dict(
	    path1,
	    "{\n"
	    "  \"CELL\": [\n"
	    "    { \"id\": \"Balmora, Guild of Fighters\", \"old\": \"\", \"new\": \"Balmora\", \"status\": \"translated\" "
	    "},\n"
	    "    { \"id\": \"Balmora, Guild of Mages\", \"old\": \"\", \"new\": \"Balmora\", \"status\": \"translated\" }\n"
	    "  ]\n"
	    "}\n");

	dict_merger_t merger({ path1 });

	std::string log = tools_t::get_log();
	REQUIRE(log.find("duplicate CELL value") != std::string::npos);
	REQUIRE(log.find("Balmora") != std::string::npos);

	remove_temp_file(path1);
}
