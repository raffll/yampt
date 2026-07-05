#include <catch2/catch_all.hpp>
#include <scanner/dial_info_align.hpp>
#include <scanner/plugin_scan.hpp>
#include <filesystem>
#include <fstream>
#include <string>

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

static std::string make_string(const std::string & text)
{
	return text + std::string(1, '\0');
}

static std::string make_tes3_record()
{
	std::string hedr(300, '\0');
	return make_record("TES3", make_sub("HEDR", hedr));
}

static std::string make_dial_record(const std::string & topic_name, uint8_t dial_type)
{
	return make_record("DIAL", make_sub("NAME", make_string(topic_name)) + make_sub("DATA", std::string(1, static_cast<char>(dial_type))));
}

static std::string make_info_record(const std::string & inam, const std::string & onam)
{
	auto subs = make_sub("INAM", make_string(inam));
	if (!onam.empty())
		subs += make_sub("ONAM", make_string(onam));

	return make_record("INFO", subs);
}

static std::string get_temp_path(const std::string & filename)
{
	return (std::filesystem::temp_directory_path() / filename).string();
}

static void write_binary_file(const std::string & path, const std::string & content)
{
	std::ofstream file(path, std::ios::binary);
	file.write(content.data(), content.size());
}

TEST_CASE("dial_info_align_t::build, collects INFOs for a DIAL", "[i]")
{
	namespace fs = std::filesystem;

	auto plugin_content = make_tes3_record()
	                    + make_dial_record("Khajiit", 0)
	                    + make_info_record("info1", "NPC_A")
	                    + make_info_record("info2", "NPC_B")
	                    + make_dial_record("Other", 0)
	                    + make_info_record("info3", "NPC_C");

	auto path = get_temp_path("yampt_test_dial_align.esm");
	write_binary_file(path, plugin_content);

	plugin_scan_t scan;
	scan.load_plugin(path);
	scan.rebuild_conflicts();

	auto result = dial_info_align_t::build(scan, "Khajiit");

	REQUIRE(result.entries.size() == 2);
	REQUIRE(result.entries[0].inam == "info1");
	REQUIRE(result.entries[1].inam == "info2");
	REQUIRE(result.entries[0].display_name == "NPC_A");
	REQUIRE(result.entries[1].display_name == "NPC_B");
	REQUIRE(result.entries[0].present_in_plugin[0] == true);
	REQUIRE(result.entries[1].present_in_plugin[0] == true);

	fs::remove(path);
}

TEST_CASE("dial_info_align_t::build, empty for unknown DIAL", "[i]")
{
	namespace fs = std::filesystem;

	auto plugin_content = make_tes3_record()
	                    + make_dial_record("Khajiit", 0)
	                    + make_info_record("info1", "");

	auto path = get_temp_path("yampt_test_dial_align2.esm");
	write_binary_file(path, plugin_content);

	plugin_scan_t scan;
	scan.load_plugin(path);
	scan.rebuild_conflicts();

	auto result = dial_info_align_t::build(scan, "NonExistent");

	REQUIRE(result.entries.empty());

	fs::remove(path);
}
