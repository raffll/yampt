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
	return make_record(
	    "DIAL",
	    make_sub("NAME", make_string(topic_name)) + make_sub("DATA", std::string(1, static_cast<char>(dial_type))));
}

static std::string make_info_record(
    const std::string & inam,
    const std::string & onam,
    const std::string & pnam = "",
    const std::string & name_text = "")
{
	auto subs = make_sub("INAM", make_string(inam));
	subs += make_sub("PNAM", make_string(pnam));
	if (!onam.empty())
		subs += make_sub("ONAM", make_string(onam));

	if (!name_text.empty())
		subs += make_sub("NAME", make_string(name_text));

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

	auto plugin_content = make_tes3_record() + make_dial_record("Khajiit", 0) + make_info_record("info1", "NPC_A", "") +
	                      make_info_record("info2", "NPC_B", "info1") + make_dial_record("Other", 0) +
	                      make_info_record("info3", "NPC_C", "");

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

TEST_CASE("dial_info_align_t::build, captures NAME response text per plugin", "[i]")
{
	namespace fs = std::filesystem;

	auto plugin_content = make_tes3_record() + make_dial_record("Greeting", 2) +
	                      make_info_record("info1", "NPC_A", "", "Hello there.") +
	                      make_info_record("info2", "NPC_B", "info1", "");

	auto path = get_temp_path("yampt_test_dial_align_text.esm");
	write_binary_file(path, plugin_content);

	plugin_scan_t scan;
	scan.load_plugin(path);
	scan.rebuild_conflicts();

	auto result = dial_info_align_t::build(scan, "Greeting");

	REQUIRE(result.entries.size() == 2);
	REQUIRE(result.entries[0].text_in_plugin[0] == "Hello there.");
	REQUIRE(result.entries[1].text_in_plugin[0].empty());

	fs::remove(path);
}

TEST_CASE("dial_info_align_t::build, empty for unknown DIAL", "[i]")
{
	namespace fs = std::filesystem;

	auto plugin_content = make_tes3_record() + make_dial_record("Khajiit", 0) + make_info_record("info1", "", "");

	auto path = get_temp_path("yampt_test_dial_align2.esm");
	write_binary_file(path, plugin_content);

	plugin_scan_t scan;
	scan.load_plugin(path);
	scan.rebuild_conflicts();

	auto result = dial_info_align_t::build(scan, "NonExistent");

	REQUIRE(result.entries.empty());

	fs::remove(path);
}

TEST_CASE("dial_info_align_t::build, plugin inserts INFO between existing", "[i]")
{
	namespace fs = std::filesystem;

	auto master_content = make_tes3_record() + make_dial_record("topic", 0) + make_info_record("A", "NPC1", "") +
	                      make_info_record("B", "NPC2", "A");

	auto plugin_content = make_tes3_record() + make_dial_record("topic", 0) + make_info_record("C", "NPC3", "A");

	auto master_path = get_temp_path("yampt_test_dial_order_master.esm");
	auto plugin_path = get_temp_path("yampt_test_dial_order_plugin.esp");
	write_binary_file(master_path, master_content);
	write_binary_file(plugin_path, plugin_content);

	plugin_scan_t scan;
	scan.load_plugin(master_path);
	scan.load_plugin(plugin_path);
	scan.rebuild_conflicts();

	auto result = dial_info_align_t::build(scan, "topic");

	REQUIRE(result.entries.size() == 3);
	REQUIRE(result.entries[0].inam == "A");
	REQUIRE(result.entries[1].inam == "C");
	REQUIRE(result.entries[2].inam == "B");

	REQUIRE(result.entries[0].source_plugin_idx == 0);
	REQUIRE(result.entries[1].source_plugin_idx == 1);
	REQUIRE(result.entries[2].source_plugin_idx == 0);

	fs::remove(master_path);
	fs::remove(plugin_path);
}

TEST_CASE("dial_info_align_t::build, plugin repositions existing INFO", "[i]")
{
	namespace fs = std::filesystem;

	auto master_content = make_tes3_record() + make_dial_record("topic", 0) + make_info_record("A", "", "") +
	                      make_info_record("B", "", "A") + make_info_record("C", "", "B");

	auto plugin_content = make_tes3_record() + make_dial_record("topic", 0) + make_info_record("B", "", "");

	auto master_path = get_temp_path("yampt_test_dial_reorder_master.esm");
	auto plugin_path = get_temp_path("yampt_test_dial_reorder_plugin.esp");
	write_binary_file(master_path, master_content);
	write_binary_file(plugin_path, plugin_content);

	plugin_scan_t scan;
	scan.load_plugin(master_path);
	scan.load_plugin(plugin_path);
	scan.rebuild_conflicts();

	auto result = dial_info_align_t::build(scan, "topic");

	REQUIRE(result.entries.size() == 3);
	REQUIRE(result.entries[0].inam == "B");
	REQUIRE(result.entries[1].inam == "A");
	REQUIRE(result.entries[2].inam == "C");

	fs::remove(master_path);
	fs::remove(plugin_path);
}

TEST_CASE("dial_info_align_t::build, unknown PNAM inserts at end", "[i]")
{
	namespace fs = std::filesystem;

	auto master_content = make_tes3_record() + make_dial_record("topic", 0) + make_info_record("A", "", "");

	auto plugin_content = make_tes3_record() + make_dial_record("topic", 0) + make_info_record("D", "", "nonexistent");

	auto master_path = get_temp_path("yampt_test_dial_unknown_pnam_master.esm");
	auto plugin_path = get_temp_path("yampt_test_dial_unknown_pnam_plugin.esp");
	write_binary_file(master_path, master_content);
	write_binary_file(plugin_path, plugin_content);

	plugin_scan_t scan;
	scan.load_plugin(master_path);
	scan.load_plugin(plugin_path);
	scan.rebuild_conflicts();

	auto result = dial_info_align_t::build(scan, "topic");

	REQUIRE(result.entries.size() == 2);
	REQUIRE(result.entries[0].inam == "A");
	REQUIRE(result.entries[1].inam == "D");

	fs::remove(master_path);
	fs::remove(plugin_path);
}
