#include <catch2/catch_all.hpp>
#include <scanner/auto_merge.hpp>
#include <scanner/plugin_scan.hpp>
#include <scanner/sub_record_merge.hpp>
#include <utility/app_logger.hpp>
#include <cstring>
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

static std::string make_record(const std::string & rec_type, const std::string & subs, uint32_t flags = 0)
{
	std::string header;
	header += rec_type;
	uint32_t body_size = static_cast<uint32_t>(subs.size());
	header.append(reinterpret_cast<const char *>(&body_size), 4);
	uint32_t header1 = 0;
	header.append(reinterpret_cast<const char *>(&header1), 4);
	header.append(reinterpret_cast<const char *>(&flags), 4);
	return header + subs;
}

static std::string make_string(const std::string & text)
{
	return text + std::string(1, '\0');
}

static std::string make_uint16(uint16_t value)
{
	std::string result(2, '\0');
	std::memcpy(result.data(), &value, 2);
	return result;
}

static std::string make_uint32(uint32_t value)
{
	std::string result(4, '\0');
	std::memcpy(result.data(), &value, 4);
	return result;
}

static std::string make_levi_entry(const std::string & item_id, uint16_t level)
{
	return make_sub("INAM", make_string(item_id)) + make_sub("INTV", make_uint16(level));
}

static std::string make_levc_entry(const std::string & creature_id, uint16_t level)
{
	return make_sub("CNAM", make_string(creature_id)) + make_sub("INTV", make_uint16(level));
}

static std::string make_levi_header(
    const std::string & list_id,
    uint32_t list_flags,
    uint8_t chance_none,
    uint32_t item_count)
{
	return make_sub("NAME", make_string(list_id)) + make_sub("DATA", make_uint32(list_flags)) +
	       make_sub("NNAM", std::string(1, static_cast<char>(chance_none))) + make_sub("INDX", make_uint32(item_count));
}

// ============================================================================
// Requirement 2: Occurrence-Counted Leveled List Merge
// ============================================================================

TEST_CASE("leveled_list_merge_t::merge, plugin count increase applied", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("iron_sword", 1);
	auto mod_subs = make_levi_header("list_id", 1, 0, 3) + make_levi_entry("iron_sword", 1) +
	                make_levi_entry("iron_sword", 1) + make_levi_entry("iron_sword", 1);
	auto winner_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("iron_sword", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
		make_record("LEVI", winner_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(result.changed);
	size_t count = 0;
	size_t search_pos = 0;
	while ((search_pos = result.content.find("INAM", search_pos)) != std::string::npos)
	{
		++count;
		search_pos += 4;
	}
	REQUIRE(count == 3);
}

TEST_CASE("leveled_list_merge_t::merge, two mods add different items", "[u]")
{
	auto header = make_levi_header("list_id", 1, 0, 1);
	auto first_subs = header + make_levi_entry("item_a", 1);
	auto mod1_subs = make_levi_header("list_id", 1, 0, 2) + make_levi_entry("item_a", 1) + make_levi_entry("item_b", 1);
	auto mod2_subs = make_levi_header("list_id", 1, 0, 2) + make_levi_entry("item_a", 1) + make_levi_entry("item_c", 5);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod1_subs),
		make_record("LEVI", mod2_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(result.content.find("item_a") != std::string::npos);
	REQUIRE(result.content.find("item_b") != std::string::npos);
	REQUIRE(result.content.find("item_c") != std::string::npos);
}

TEST_CASE("leveled_list_merge_t::merge, sorted by level then ident", "[u]")
{
	auto header = make_levi_header("list_id", 1, 0, 1);
	auto first_subs = header + make_levi_entry("zzz_item", 1);
	auto mod_subs = make_levi_header("list_id", 1, 0, 3) + make_levi_entry("zzz_item", 1) +
	                make_levi_entry("aaa_item", 5) + make_levi_entry("bbb_item", 3);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(result.changed);
	size_t pos_zzz = result.content.find("zzz_item");
	size_t pos_bbb = result.content.find("bbb_item");
	size_t pos_aaa = result.content.find("aaa_item");
	REQUIRE(pos_zzz < pos_bbb);
	REQUIRE(pos_bbb < pos_aaa);
}

TEST_CASE("leveled_list_merge_t::merge, winner header used", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("item_a", 1);
	auto winner_subs = make_levi_header("list_id", 3, 50, 1) + make_levi_entry("item_a", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", winner_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	size_t nnam_pos = result.content.find("NNAM");
	REQUIRE(nnam_pos != std::string::npos);
	uint8_t chance = static_cast<uint8_t>(result.content[nnam_pos + 8]);
	REQUIRE(chance == 50);
}

// ============================================================================
// Requirement 15: Leveled List Deletion Detection
// ============================================================================

TEST_CASE("leveled_list_merge_t::merge, mod removes item entirely", "[u]")
{
	auto first_subs =
	    make_levi_header("list_id", 1, 0, 2) + make_levi_entry("keep_item", 1) + make_levi_entry("remove_me", 1);
	auto mod_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("keep_item", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(result.content.find("keep_item") != std::string::npos);
	REQUIRE(result.content.find("remove_me") == std::string::npos);
}

TEST_CASE("leveled_list_merge_t::merge, mod reduces count partial", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 3) + make_levi_entry("item_a", 1) +
	                  make_levi_entry("item_a", 1) + make_levi_entry("item_a", 1);
	auto mod_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("item_a", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	size_t count = 0;
	size_t search_pos = 0;
	while ((search_pos = result.content.find("item_a", search_pos)) != std::string::npos)
	{
		++count;
		search_pos += 6;
	}
	REQUIRE(count == 1);
}

TEST_CASE("leveled_list_merge_t::merge, one mod removes one mod keeps deletion wins", "[u]")
{
	auto first_subs =
	    make_levi_header("list_id", 1, 0, 2) + make_levi_entry("contested", 1) + make_levi_entry("safe_item", 1);
	auto mod_remove = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("safe_item", 1);
	auto mod_keep = make_levi_header("list_id", 1, 0, 3) + make_levi_entry("contested", 1) +
	                make_levi_entry("contested", 1) + make_levi_entry("safe_item", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_remove),
		make_record("LEVI", mod_keep),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(result.content.find("contested") == std::string::npos);
	REQUIRE(result.content.find("safe_item") != std::string::npos);
}

TEST_CASE("leveled_list_merge_t::merge, no deletion when all mods keep item", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("item_a", 1);
	auto mod1_subs = make_levi_header("list_id", 1, 0, 2) + make_levi_entry("item_a", 1) + make_levi_entry("item_a", 1);
	auto mod2_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("item_a", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod1_subs),
		make_record("LEVI", mod2_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(result.content.find("item_a") != std::string::npos);
}

TEST_CASE("leveled_list_merge_t::merge, LEVC creature list works same", "[u]")
{
	auto first_subs = make_levi_header("crea_list", 1, 0, 1) + make_levc_entry("rat", 1);
	auto mod_subs = make_levi_header("crea_list", 1, 0, 2) + make_levc_entry("rat", 1) + make_levc_entry("mudcrab", 3);
	auto winner_subs = make_levi_header("crea_list", 1, 0, 1) + make_levc_entry("rat", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVC";
	input.record_id = "crea_list";
	input.version_contents = {
		make_record("LEVC", first_subs),
		make_record("LEVC", mod_subs),
		make_record("LEVC", winner_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(result.content.find("rat") != std::string::npos);
	REQUIRE(result.content.find("mudcrab") != std::string::npos);
}

// ============================================================================
// Per-occurrence PC level merge (item ID keyed, level merges by precedence)
// ============================================================================

static uint16_t read_entry_level(const std::string & content, const std::string & item_id)
{
	const auto id_pos = content.find(item_id);
	REQUIRE(id_pos != std::string::npos);

	const auto intv_pos = content.find("INTV", id_pos);
	REQUIRE(intv_pos != std::string::npos);

	uint16_t level = 0;
	std::memcpy(&level, content.data() + intv_pos + 8, 2);
	return level;
}

static size_t count_occurrences(const std::string & content, const std::string & needle)
{
	size_t count = 0;
	size_t search_pos = 0;
	while ((search_pos = content.find(needle, search_pos)) != std::string::npos)
	{
		++count;
		search_pos += needle.size();
	}

	return count;
}

TEST_CASE("leveled_list_merge_t::merge, plugin changes entry PC level wins", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("dwe_long", 13);
	auto mod_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("dwe_long", 14);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(count_occurrences(result.content, "dwe_long") == 1);
	REQUIRE(read_entry_level(result.content, "dwe_long") == 14);
}

TEST_CASE("leveled_list_merge_t::merge, unchanged level keeps master value", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("dwe_long", 13);
	auto mod_subs = make_levi_header("list_id", 1, 0, 2) + make_levi_entry("dwe_long", 13) +
	                make_levi_entry("new_item", 5);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(read_entry_level(result.content, "dwe_long") == 13);
	REQUIRE(read_entry_level(result.content, "new_item") == 5);
}

TEST_CASE("leveled_list_merge_t::merge, last plugin wins on level conflict", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("dwe_long", 13);
	auto mod1_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("dwe_long", 20);
	auto mod2_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("dwe_long", 30);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod1_subs),
		make_record("LEVI", mod2_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(count_occurrences(result.content, "dwe_long") == 1);
	REQUIRE(read_entry_level(result.content, "dwe_long") == 30);
}

TEST_CASE("leveled_list_merge_t::merge, duplicate weighting preserved not collapsed", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 3) + make_levi_entry("iron_sword", 1) +
	                  make_levi_entry("iron_sword", 1) + make_levi_entry("iron_sword", 1);
	auto mod_subs = make_levi_header("list_id", 1, 0, 3) + make_levi_entry("iron_sword", 1) +
	                make_levi_entry("iron_sword", 1) + make_levi_entry("iron_sword", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(count_occurrences(result.content, "iron_sword") == 3);
}

// ============================================================================
// Occurrence count resolved as a conflict (last plugin that changed it wins)
// ============================================================================

TEST_CASE("leveled_list_merge_t::merge, count change wins over master", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("iron_sword", 1);
	auto mod_subs = make_levi_header("list_id", 1, 0, 3) + make_levi_entry("iron_sword", 1) +
	                make_levi_entry("iron_sword", 1) + make_levi_entry("iron_sword", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(count_occurrences(result.content, "iron_sword") == 3);
}

TEST_CASE("leveled_list_merge_t::merge, two mods same count is not summed", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 0);
	auto mod1_subs = make_levi_header("list_id", 1, 0, 3) + make_levi_entry("iron_sword", 1) +
	                 make_levi_entry("iron_sword", 1) + make_levi_entry("iron_sword", 1);
	auto mod2_subs = make_levi_header("list_id", 1, 0, 3) + make_levi_entry("iron_sword", 1) +
	                 make_levi_entry("iron_sword", 1) + make_levi_entry("iron_sword", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod1_subs),
		make_record("LEVI", mod2_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(count_occurrences(result.content, "iron_sword") == 3);
}

TEST_CASE("leveled_list_merge_t::merge, last plugin count wins on conflict", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("iron_sword", 1);
	auto mod1_subs = make_levi_header("list_id", 1, 0, 2) + make_levi_entry("iron_sword", 1) +
	                 make_levi_entry("iron_sword", 1);
	auto mod2_subs = make_levi_header("list_id", 1, 0, 5) + make_levi_entry("iron_sword", 1) +
	                 make_levi_entry("iron_sword", 1) + make_levi_entry("iron_sword", 1) +
	                 make_levi_entry("iron_sword", 1) + make_levi_entry("iron_sword", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod1_subs),
		make_record("LEVI", mod2_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(count_occurrences(result.content, "iron_sword") == 5);
}

TEST_CASE("leveled_list_merge_t::merge, unchanged count keeps master", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 2) + make_levi_entry("iron_sword", 1) +
	                  make_levi_entry("iron_sword", 1);
	auto mod1_subs = make_levi_header("list_id", 1, 0, 2) + make_levi_entry("iron_sword", 1) +
	                 make_levi_entry("iron_sword", 1);
	auto mod2_subs = make_levi_header("list_id", 1, 0, 2) + make_levi_entry("iron_sword", 1) +
	                 make_levi_entry("iron_sword", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod1_subs),
		make_record("LEVI", mod2_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(count_occurrences(result.content, "iron_sword") == 2);
}

TEST_CASE("leveled_list_merge_t::merge, added item count from last plugin", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 0);
	auto mod1_subs = make_levi_header("list_id", 1, 0, 3) + make_levi_entry("new_item", 1) +
	                 make_levi_entry("new_item", 1) + make_levi_entry("new_item", 1);
	auto mod2_subs = make_levi_header("list_id", 1, 0, 2) + make_levi_entry("new_item", 1) +
	                 make_levi_entry("new_item", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod1_subs),
		make_record("LEVI", mod2_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(count_occurrences(result.content, "new_item") == 2);
}

// ============================================================================
// End-to-end merge through real plugin files on disk
// ============================================================================

static std::string make_tes3_header_record()
{
	std::string hedr(300, '\0');
	return make_record("TES3", make_sub("HEDR", hedr));
}

static void write_plugin_file(const std::string & path, const std::string & body)
{
	std::ofstream file(path, std::ios::binary);
	file.write(body.data(), body.size());
}

static std::string temp_plugin_path(const std::string & filename)
{
	return (std::filesystem::temp_directory_path() / filename).string();
}

TEST_CASE("auto_merge_t::execute, leveled list entry level merges from plugin file", "[i]")
{
	namespace fs = std::filesystem;

	const auto master_body = make_tes3_header_record() +
	                         make_record("LEVI", make_levi_header("test_list", 1, 0, 1) +
	                                                 make_levi_entry("dwe_long", 13));
	const auto plugin_body = make_tes3_header_record() +
	                         make_record("LEVI", make_levi_header("test_list", 1, 0, 1) +
	                                                 make_levi_entry("dwe_long", 14));
	const auto revert_body = make_tes3_header_record() +
	                         make_record("LEVI", make_levi_header("test_list", 1, 0, 1) +
	                                                 make_levi_entry("dwe_long", 13));

	const auto master_path = temp_plugin_path("yampt_levi_master.esm");
	const auto plugin_path = temp_plugin_path("yampt_levi_plugin.esp");
	const auto revert_path = temp_plugin_path("yampt_levi_revert.esp");
	write_plugin_file(master_path, master_body);
	write_plugin_file(plugin_path, plugin_body);
	write_plugin_file(revert_path, revert_body);

	plugin_scan_t scan;
	scan.load_plugin(master_path);
	scan.load_plugin(plugin_path);
	scan.load_plugin(revert_path);
	scan.set_active_plugin("Merged Patch.esp");
	scan.rebuild_conflicts();

	auto_merge_t merge(scan);
	merge.execute();

	const auto * merged = scan.find_active_content("LEVI", "test_list");
	REQUIRE(merged != nullptr);

	const auto intv_pos = merged->find("INTV");
	REQUIRE(intv_pos != std::string::npos);
	uint16_t level = 0;
	std::memcpy(&level, merged->data() + intv_pos + 8, 2);
	REQUIRE(level == 14);

	const auto second_intv = merged->find("INTV", intv_pos + 1);
	REQUIRE(second_intv == std::string::npos);

	fs::remove(master_path);
	fs::remove(plugin_path);
	fs::remove(revert_path);
}

// ============================================================================
// Leveled list DATA flags merge per bit
// ============================================================================

static uint32_t read_data_flags(const std::string & content)
{
	const auto data_pos = content.find("DATA");
	REQUIRE(data_pos != std::string::npos);

	uint32_t flags = 0;
	std::memcpy(&flags, content.data() + data_pos + 8, 4);
	return flags;
}

TEST_CASE("leveled_list_merge_t::merge, DATA flags merge per bit", "[u]")
{
	auto first_subs = make_levi_header("list_id", 0, 0, 1) + make_levi_entry("item_a", 1);
	auto mod_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("item_a", 1);
	auto winner_subs = make_levi_header("list_id", 2, 0, 1) + make_levi_entry("item_a", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
		make_record("LEVI", winner_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(read_data_flags(result.content) == 3);
}

TEST_CASE("leveled_list_merge_t::merge, DATA unchanged flag keeps master bit", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("item_a", 1);
	auto mod_subs = make_levi_header("list_id", 3, 0, 2) + make_levi_entry("item_a", 1) +
	                make_levi_entry("item_b", 2);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(read_data_flags(result.content) == 3);
}

TEST_CASE("auto_merge_t::execute, leveled list DATA flags merge per bit from plugin files", "[i]")
{
	namespace fs = std::filesystem;

	const auto master_body = make_tes3_header_record() +
	                         make_record("LEVI", make_levi_header("flag_list", 0, 0, 1) +
	                                                 make_levi_entry("item_a", 1));
	const auto plugin1_body = make_tes3_header_record() +
	                          make_record("LEVI", make_levi_header("flag_list", 1, 0, 1) +
	                                                  make_levi_entry("item_a", 1));
	const auto plugin2_body = make_tes3_header_record() +
	                          make_record("LEVI", make_levi_header("flag_list", 2, 0, 1) +
	                                                  make_levi_entry("item_a", 1));

	const auto master_path = temp_plugin_path("yampt_levi_flag_master.esm");
	const auto plugin1_path = temp_plugin_path("yampt_levi_flag_p1.esp");
	const auto plugin2_path = temp_plugin_path("yampt_levi_flag_p2.esp");
	write_plugin_file(master_path, master_body);
	write_plugin_file(plugin1_path, plugin1_body);
	write_plugin_file(plugin2_path, plugin2_body);

	plugin_scan_t scan;
	scan.load_plugin(master_path);
	scan.load_plugin(plugin1_path);
	scan.load_plugin(plugin2_path);
	scan.set_active_plugin("Merged Patch.esp");
	scan.rebuild_conflicts();

	auto_merge_t merge(scan);
	merge.execute();

	const auto * merged = scan.find_active_content("LEVI", "flag_list");
	REQUIRE(merged != nullptr);
	REQUIRE(read_data_flags(*merged) == 3);

	fs::remove(master_path);
	fs::remove(plugin1_path);
	fs::remove(plugin2_path);
}

static std::string make_npc_record(const std::string & npc_id, uint32_t flag_value)
{
	return make_record(
	    "NPC_",
	    make_sub("NAME", make_string(npc_id)) + make_sub("FLAG", make_uint32(flag_value)));
}

static uint32_t read_npc_flag(const std::string & content)
{
	const auto flag_pos = content.find("FLAG");
	REQUIRE(flag_pos != std::string::npos);

	uint32_t value = 0;
	std::memcpy(&value, content.data() + flag_pos + 8, 4);
	return value;
}

TEST_CASE("auto_merge_t::execute, NPC FLAG merges per bit from plugin files", "[i]")
{
	namespace fs = std::filesystem;

	const auto master_body = make_tes3_header_record() + make_npc_record("guard", 0);
	const auto plugin1_body = make_tes3_header_record() + make_npc_record("guard", 0x0001);
	const auto plugin2_body = make_tes3_header_record() + make_npc_record("guard", 0x0002);

	const auto master_path = temp_plugin_path("yampt_npc_flag_master.esm");
	const auto plugin1_path = temp_plugin_path("yampt_npc_flag_p1.esp");
	const auto plugin2_path = temp_plugin_path("yampt_npc_flag_p2.esp");
	write_plugin_file(master_path, master_body);
	write_plugin_file(plugin1_path, plugin1_body);
	write_plugin_file(plugin2_path, plugin2_body);

	plugin_scan_t scan;
	scan.load_plugin(master_path);
	scan.load_plugin(plugin1_path);
	scan.load_plugin(plugin2_path);
	scan.set_active_plugin("Merged Patch.esp");
	scan.rebuild_conflicts();

	auto_merge_t merge(scan);
	merge.execute();

	const auto * merged = scan.find_active_content("NPC_", "guard");
	REQUIRE(merged != nullptr);
	REQUIRE(read_npc_flag(*merged) == 0x0003);

	fs::remove(master_path);
	fs::remove(plugin1_path);
	fs::remove(plugin2_path);
}

// ============================================================================
// Landscape (LAND) is never written to the merged patch
// ============================================================================

static std::string make_land(int32_t grid_x, int32_t grid_y, const std::string & height_data)
{
	std::string intv;
	intv.append(reinterpret_cast<const char *>(&grid_x), 4);
	intv.append(reinterpret_cast<const char *>(&grid_y), 4);

	auto body = make_sub("INTV", intv);
	body += make_sub("DATA", make_uint32(1));
	body += make_sub("VHGT", height_data);
	return make_record("LAND", body);
}

TEST_CASE("auto_merge_t::execute, LAND record excluded from merged patch", "[i]")
{
	namespace fs = std::filesystem;

	const auto master_body = make_tes3_header_record() + make_land(0, 0, std::string(64, '\x01'));
	const auto plugin_body = make_tes3_header_record() + make_land(0, 0, std::string(64, '\x02'));

	const auto master_path = temp_plugin_path("yampt_land_master.esm");
	const auto plugin_path = temp_plugin_path("yampt_land_plugin.esp");
	write_plugin_file(master_path, master_body);
	write_plugin_file(plugin_path, plugin_body);

	plugin_scan_t scan;
	scan.load_plugin(master_path);
	scan.load_plugin(plugin_path);
	scan.set_active_plugin("Merged Patch.esp");
	scan.rebuild_conflicts();

	auto_merge_t merge(scan);
	merge.execute();

	REQUIRE(scan.find_active_content("LAND", "GRID[0,0]") == nullptr);

	fs::remove(master_path);
	fs::remove(plugin_path);
}
