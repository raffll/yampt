#include <catch2/catch_all.hpp>
#include <editor/inflection_store.hpp>
#include <io/codepage.hpp>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::string create_temp_top(const std::string & name, const std::string & content)
{
	const auto path = (std::filesystem::temp_directory_path() / name).string();
	std::ofstream stream(path, std::ios::binary);
	stream << content;
	stream.close();
	return path;
}

} // namespace

TEST_CASE("inflection_store_t::annotate, keeps forms present in translation text", "[i]")
{
	const auto path = create_temp_top("infl_present.top", "miecze\tmiecz\r\nmieczy\tmiecz\r\n");

	inflection_store_t store;
	store.rebuild({ path }, codepage_t::windows_1250);

	const auto result = store.annotate("Dwa miecze i cztery topory");

	REQUIRE(result.size() == 1);
	REQUIRE(result[0].kind == annotation_t::inflection_form);
	REQUIRE(result[0].old_text == "miecze");
	REQUIRE(result[0].new_text == "miecz");
	REQUIRE(result[0].source == "infl_present.top");

	std::filesystem::remove(path);
}

TEST_CASE("inflection_store_t::annotate, excludes forms absent from translation text", "[i]")
{
	const auto path = create_temp_top("infl_absent.top", "miecze\tmiecz\r\nmieczy\tmiecz\r\n");

	inflection_store_t store;
	store.rebuild({ path }, codepage_t::windows_1250);

	const auto result = store.annotate("Nie ma tutaj zadnej broni");

	REQUIRE(result.empty());

	std::filesystem::remove(path);
}

TEST_CASE("inflection_store_t::annotate, match is case insensitive", "[i]")
{
	const auto path = create_temp_top("infl_case.mrk", "Balmora\tBalmora PL\r\n");

	inflection_store_t store;
	store.rebuild({ path }, codepage_t::windows_1250);

	const auto result = store.annotate("wracam do balmora o zmierzchu");

	REQUIRE(result.size() == 1);
	REQUIRE(result[0].old_text == "Balmora");
	REQUIRE(result[0].new_text == "Balmora PL");

	std::filesystem::remove(path);
}

TEST_CASE("inflection_store_t::annotate, cel entries produce nothing", "[i]")
{
	const auto path = create_temp_top("infl_cel.cel", "Balmora\tBalmora PL\r\n");

	inflection_store_t store;
	store.rebuild({ path }, codepage_t::windows_1250);

	const auto result = store.annotate("Balmora");

	REQUIRE(result.empty());

	std::filesystem::remove(path);
}

TEST_CASE("inflection_store_t::annotate, empty translation text produces nothing", "[i]")
{
	const auto path = create_temp_top("infl_empty.top", "miecze\tmiecz\r\n");

	inflection_store_t store;
	store.rebuild({ path }, codepage_t::windows_1250);

	const auto result = store.annotate("");

	REQUIRE(result.empty());

	std::filesystem::remove(path);
}

TEST_CASE("inflection_store_t::clear, removes all entries", "[i]")
{
	const auto path = create_temp_top("infl_clear.top", "miecze\tmiecz\r\n");

	inflection_store_t store;
	store.rebuild({ path }, codepage_t::windows_1250);
	store.clear();

	const auto result = store.annotate("Dwa miecze");

	REQUIRE(result.empty());

	std::filesystem::remove(path);
}
