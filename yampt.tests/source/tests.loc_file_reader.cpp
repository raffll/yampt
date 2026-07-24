#include <catch2/catch_test_macros.hpp>
#include <io/loc_file_reader.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static std::string temp_path(const std::string & name)
{
	auto path = (fs::temp_directory_path() / name).string();
	return path;
}

static void write_bytes(const std::string & path, const std::string & content)
{
	std::ofstream stream(path, std::ios::binary);
	stream.write(content.data(), content.size());
}

static void cleanup(const std::string & path)
{
	std::error_code error_code;
	fs::remove(path, error_code);
}

TEST_CASE("loc_file_reader::read, parses tab-separated entries", "[i]")
{
	const auto path = temp_path("yampt_test_loc_reader.cel");

	const std::string raw_content =
		"Balmora\tBalmora_PL\r\n"
		"Vivec\tVivec_PL\r\n"
		"\r\n"
		"Ald-ruhn\tStary Ruhn\r\n";

	write_bytes(path, raw_content);
	REQUIRE(fs::exists(path));

	const auto result = loc_file_reader::read(path, codepage_t::windows_1252);

	REQUIRE(result.file_kind == loc_types::loc_file_kind_t::cel);
	REQUIRE(result.entries.size() == 3);

	REQUIRE(result.entries[0].key == "Balmora");
	REQUIRE(result.entries[0].value == "Balmora_PL");

	REQUIRE(result.entries[1].key == "Vivec");
	REQUIRE(result.entries[1].value == "Vivec_PL");

	REQUIRE(result.entries[2].key == "Ald-ruhn");
	REQUIRE(result.entries[2].value == "Stary Ruhn");

	cleanup(path);
}

TEST_CASE("loc_file_reader::classify_extension, identifies cel/top/mrk", "[u]")
{
	using loc_types::loc_file_kind_t;

	REQUIRE(loc_file_reader::classify_extension("cells.cel") == loc_file_kind_t::cel);
	REQUIRE(loc_file_reader::classify_extension("topics.top") == loc_file_kind_t::top);
	REQUIRE(loc_file_reader::classify_extension("marks.mrk") == loc_file_kind_t::mrk);
	REQUIRE(loc_file_reader::classify_extension("UPPER.CEL") == loc_file_kind_t::cel);
	REQUIRE(loc_file_reader::classify_extension("Mixed.ToP") == loc_file_kind_t::top);
	REQUIRE(loc_file_reader::classify_extension("file.xyz") == loc_file_kind_t::cel);
}
