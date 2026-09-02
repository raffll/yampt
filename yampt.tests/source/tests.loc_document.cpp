#include <catch2/catch_all.hpp>
#include <model/loc_document.hpp>
#include <io/codepage.hpp>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::string create_temp_loc(const std::string & name, const std::string & content)
{
	const auto path = (std::filesystem::temp_directory_path() / name).string();
	std::ofstream stream(path, std::ios::binary);
	stream << content;
	stream.close();
	return path;
}

} // namespace

TEST_CASE("loc_document_t::supported_statuses, loc files expose no statuses", "[i]")
{
	const auto path = create_temp_loc("locdoc_status.top", "szczura\tszczur\r\n");

	loc_document_t document(path, codepage_t::windows_1250);

	REQUIRE(document.supported_statuses().empty());

	std::filesystem::remove(path);
}

TEST_CASE("loc_document_t::build_rows, maps entries to rows", "[i]")
{
	const auto path = create_temp_loc("locdoc_rows.top", "szczura\tszczur\r\nmieczy\tmiecz\r\n");

	loc_document_t document(path, codepage_t::windows_1250);
	const auto rows = document.build_rows();

	REQUIRE(rows.size() == 2);
	REQUIRE(rows[0].old_text == "szczura");
	REQUIRE(rows[0].new_text == "szczur");
	REQUIRE(rows[1].old_text == "mieczy");
	REQUIRE(rows[1].new_text == "miecz");

	std::filesystem::remove(path);
}

TEST_CASE("loc_document_t::kind, reports top kind", "[i]")
{
	const auto path = create_temp_loc("locdoc_kind.top", "szczura\tszczur\r\n");

	loc_document_t document(path, codepage_t::windows_1250);

	REQUIRE(document.kind() == document_kind_t::loc_top);

	std::filesystem::remove(path);
}
