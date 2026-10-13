#include <catch2/catch_all.hpp>
#include <scanner/plugin_scan.hpp>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

static std::string make_sub(const std::string & type, const std::string & data)
{
	std::string result;
	result += type;
	uint32_t size_val = static_cast<uint32_t>(data.size());
	result.append(reinterpret_cast<const char *>(&size_val), 4);
	result += data;
	return result;
}

static std::string make_record(const std::string & rec_type, const std::string & subs)
{
	std::string header;
	header += rec_type;
	uint32_t body_size = static_cast<uint32_t>(subs.size());
	header.append(reinterpret_cast<const char *>(&body_size), 4);
	uint32_t zero = 0;
	header.append(reinterpret_cast<const char *>(&zero), 4);
	header.append(reinterpret_cast<const char *>(&zero), 4);
	return header + subs;
}

static std::string make_minimal_esm()
{
	const std::string hedr(300, '\0');
	return make_record("TES3", make_sub("HEDR", hedr));
}

static std::string write_temp_esm(const std::string & filename)
{
	namespace fs = std::filesystem;
	const auto path = (fs::temp_directory_path() / filename).string();
	std::ofstream file(path, std::ios::binary);
	const auto content = make_minimal_esm();
	file.write(content.data(), static_cast<std::streamsize>(content.size()));
	return path;
}

static void remove_temp(const std::string & path)
{
	std::error_code error_code;
	std::filesystem::remove(path, error_code);
}

static bool has_directory_separator(const std::string & path)
{
	return path.find_first_of("/\\") != std::string::npos;
}

} // namespace

TEST_CASE("plugin_scan_t::plugin_path, loaded plugin has full absolute path", "[i]")
{
	const auto path = write_temp_esm("yampt_save_path_test.esm");

	plugin_scan_t scan;
	scan.load_plugin(path);
	scan.set_active_from_loaded(0);

	const int active_idx = scan.active_plugin_index();
	REQUIRE(active_idx == 0);
	REQUIRE(has_directory_separator(scan.plugin_path(active_idx)));

	remove_temp(path);
}

TEST_CASE("plugin_scan_t::plugin_path, new plugin has bare filename without directory", "[u]")
{
	plugin_scan_t scan;
	scan.set_active_plugin("Merged Patch.esp");

	const int active_idx = scan.active_plugin_index();
	REQUIRE(active_idx >= 0);
	REQUIRE_FALSE(has_directory_separator(scan.plugin_path(active_idx)));
}

TEST_CASE("plugin_scan_t::plugin_path, loaded plugin path survives active switch", "[i]")
{
	const auto path = write_temp_esm("yampt_save_path_test2.esm");

	plugin_scan_t scan;
	scan.load_plugin(path);
	scan.set_active_from_loaded(0);

	const auto stored_path = scan.plugin_path(scan.active_plugin_index());
	REQUIRE(stored_path == path);

	remove_temp(path);
}
