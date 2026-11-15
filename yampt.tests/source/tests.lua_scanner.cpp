#include <catch2/catch_all.hpp>
#include <scanner/lua_scanner.hpp>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

struct temp_data_dir_t
{
	std::filesystem::path root = std::filesystem::temp_directory_path() / "yampt_lua_scan_root";

	temp_data_dir_t()
	{
		std::filesystem::remove_all(root);
		std::filesystem::create_directories(root / "examples");
	}

	~temp_data_dir_t()
	{
		std::filesystem::remove_all(root);
	}

	void write(const std::filesystem::path & relative_path, const std::string & content) const
	{
		std::ofstream file(root / relative_path, std::ios::binary);
		file << content;
	}
};

bool any_warning_mentions(const std::vector<std::string> & warnings, const std::string & filename)
{
	return std::any_of(
	    warnings.begin(),
	    warnings.end(),
	    [&filename](const std::string & warning) { return warning.find(filename) != std::string::npos; });
}

} // namespace

TEST_CASE("lua_scanner_t::scan, reads omwscripts at the data folder root only", "[i]")
{
	const temp_data_dir_t data_dir;
	data_dir.write("root_level.omwscripts", "malformed line without separator\n");
	data_dir.write("examples/nested_fixture.omwscripts", "malformed line without separator\n");

	lua_scanner_t scanner;
	const auto result = scanner.scan({ data_dir.root.string() }, { "test mod" });

	REQUIRE(any_warning_mentions(result.warnings, "root_level.omwscripts"));
	REQUIRE_FALSE(any_warning_mentions(result.warnings, "nested_fixture.omwscripts"));
}

TEST_CASE("lua_scanner_t::scan, root stub resolves to its real omwscripts file once", "[i]")
{
	const temp_data_dir_t data_dir;
	data_dir.write("stubbed.omwscripts", "malformed line without separator\n");
	data_dir.write("stubbed.omwscripts.esp", "");

	lua_scanner_t scanner;
	const auto result = scanner.scan({ data_dir.root.string() }, { "test mod" });

	const auto mentions = std::count_if(
	    result.warnings.begin(),
	    result.warnings.end(),
	    [](const std::string & warning) { return warning.find("stubbed.omwscripts\"") != std::string::npos; });
	REQUIRE(mentions == 1);
}
