#include <catch2/catch_test_macros.hpp>
#include <io/loc_file_writer.hpp>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static std::string temp_path(const std::string & name)
{
	auto path = (fs::temp_directory_path() / name).string();
	return path;
}

static std::string read_bytes(const std::string & path)
{
	std::ifstream stream(path, std::ios::binary);
	return { std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>() };
}

static void cleanup(const std::string & path)
{
	std::error_code error_code;
	fs::remove(path, error_code);
}

TEST_CASE("loc_file_writer::write, produces correct tab-separated output", "[i]")
{
	const auto path = temp_path("yampt_test_loc_writer.cel");

	std::vector<loc_types::loc_entry_t> entries = {
		{ "Balmora", "Balmora_PL" },
		{ "Vivec", "Vivec_PL" },
		{ "Ald-ruhn", "Stary Ruhn" },
	};

	loc_file_writer::write(path, entries);
	REQUIRE(fs::exists(path));

	const auto content = read_bytes(path);

	const std::string expected =
		"Balmora\tBalmora_PL\r\n"
		"Vivec\tVivec_PL\r\n"
		"Ald-ruhn\tStary Ruhn\r\n";

	REQUIRE(content == expected);

	REQUIRE(content.substr(0, 3) != "\xEF\xBB\xBF");

	cleanup(path);
}
