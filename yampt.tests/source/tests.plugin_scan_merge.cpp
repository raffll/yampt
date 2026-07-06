#include <catch2/catch_all.hpp>
#include <utility/string_utils.hpp>
#include <utility/app_logger.hpp>
#include <scanner/plugin_scan.hpp>
#include <scanner/sub_record_merge.hpp>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>

static std::string make_sub_record(const std::string & sub_id, const std::string & content)
{
	std::string result;
	result += sub_id;
	result += domain_types::convert_uint_to_string_byte_array(content.size());
	result += content;
	return result;
}

static std::string make_record(const std::string & rec_id, const std::string & sub_records)
{
	std::string header;
	header += rec_id;
	header += domain_types::convert_uint_to_string_byte_array(sub_records.size());
	header += domain_types::convert_uint_to_string_byte_array(0);
	header += domain_types::convert_uint_to_string_byte_array(0);
	return header + sub_records;
}

static std::string make_tes3_record()
{
	return make_record("TES3", make_sub_record("HEDR", std::string(300, '\0')));
}

static std::string make_intv(uint16_t level)
{
	return std::string(reinterpret_cast<const char *>(&level), 2);
}

static std::string make_levi_record(
    const std::string & list_name,
    uint32_t flags,
    uint8_t chance_none,
    const std::vector<std::pair<std::string, uint16_t>> & items)
{
	std::string body;
	body += make_sub_record("NAME", list_name + '\0');

	std::string data_content(4, '\0');
	std::memcpy(&data_content[0], &flags, 4);
	body += make_sub_record("DATA", data_content);

	std::string nnam_content(1, '\0');
	nnam_content[0] = static_cast<char>(chance_none);
	body += make_sub_record("NNAM", nnam_content);

	uint32_t item_count = static_cast<uint32_t>(items.size());
	std::string indx_content(4, '\0');
	std::memcpy(&indx_content[0], &item_count, 4);
	body += make_sub_record("INDX", indx_content);

	for (const auto & [item_id, level] : items)
	{
		body += make_sub_record("INAM", item_id + '\0');
		body += make_sub_record("INTV", make_intv(level));
	}

	return make_record("LEVI", body);
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

static std::vector<std::pair<std::string, uint16_t>> extract_items_from_content(const std::string & content)
{
	std::vector<std::pair<std::string, uint16_t>> items;
	size_t pos = 16;
	std::string current_id;

	while (pos + 8 <= content.size())
	{
		std::string sub_type = content.substr(pos, 4);
		uint32_t sub_size = 0;
		std::memcpy(&sub_size, content.data() + pos + 4, 4);
		if (sub_size == 0)
			sub_size = 1;

		if (pos + 8 + sub_size > content.size())
			break;

		if (sub_type == "INAM" || sub_type == "CNAM")
		{
			std::string raw(content.data() + pos + 8, sub_size);
			current_id = string_utils::erase_null_chars(raw);
		}
		else if (sub_type == "INTV" && !current_id.empty())
		{
			uint16_t level = 0;
			if (sub_size >= 2)
				std::memcpy(&level, content.data() + pos + 8, 2);
			items.push_back({ current_id, level });
			current_id.clear();
		}

		pos += 8 + sub_size;
	}

	return items;
}

static std::string find_sub_record(const std::string & content, const std::string & target_type)
{
	size_t pos = 16;

	while (pos + 8 <= content.size())
	{
		std::string sub_type = content.substr(pos, 4);
		uint32_t sub_size = 0;
		std::memcpy(&sub_size, content.data() + pos + 4, 4);
		if (sub_size == 0)
			sub_size = 1;

		if (pos + 8 + sub_size > content.size())
			break;

		if (sub_type == target_type)
			return std::string(content.data() + pos + 8, sub_size);

		pos += 8 + sub_size;
	}

	return {};
}

struct merge_test_fixture_t
{
	plugin_scan_t scan;
	std::string master_path;
	std::string plugin_path;

	merge_test_fixture_t(const std::string & master_levi, const std::string & plugin_levi)
	{
		master_path = get_temp_path("yampt_merge_test_master.esm");
		plugin_path = get_temp_path("yampt_merge_test_plugin.esp");

		std::string master_file = make_tes3_record() + master_levi;
		std::string plugin_file = make_tes3_record() + plugin_levi;

		write_binary_file(master_path, master_file);
		write_binary_file(plugin_path, plugin_file);

		scan.load_plugin(master_path);
		scan.load_plugin(plugin_path);
		scan.set_merge_plugin("Merged Patch.esp");
	}

	~merge_test_fixture_t()
	{
		std::filesystem::remove(master_path);
		std::filesystem::remove(plugin_path);
	}

	conflict_entry_t make_conflict(const std::string & list_name)
	{
		conflict_entry_t entry;
		entry.rec_type = "LEVI";
		entry.record_id = list_name;

		record_version_t master_ver;
		master_ver.plugin_idx = 0;
		master_ver.record_index = 1;
		entry.versions.push_back(master_ver);

		record_version_t plugin_ver;
		plugin_ver.plugin_idx = 1;
		plugin_ver.record_index = 1;
		entry.versions.push_back(plugin_ver);

		return entry;
	}

	const std::string & merged_content()
	{
		return scan.merge_record_content(0);
	}

	void merge_leveled_list(const conflict_entry_t & entry)
	{
		std::vector<std::string> version_contents;
		for (const auto & ver : entry.versions)
			version_contents.push_back(scan.read_record_content(ver.plugin_idx, ver.record_index));

		merge_input_t input;
		input.rec_type = entry.rec_type;
		input.record_id = entry.record_id;
		input.version_contents = std::move(version_contents);

		const auto result = leveled_list_merge_t::merge(input);
		if (!result.content.empty())
			scan.copy_record_to_merge_raw(entry.rec_type, entry.record_id, result.content);
	}
};

TEST_CASE("plugin_scan_t::merge_leveled_list, additions-only preserving master", "[i]")
{
	auto master_levi = make_levi_record(
	    "lev_item_test",
	    1,
	    0,
	    {
	        { "iron_sword", 1 },
	        { "steel_dagger", 3 },
	    });

	auto plugin_levi = make_levi_record(
	    "lev_item_test",
	    1,
	    0,
	    {
	        { "iron_sword", 1 },
	        { "steel_dagger", 3 },
	        { "glass_blade", 5 },
	    });

	merge_test_fixture_t fixture(master_levi, plugin_levi);
	auto entry = fixture.make_conflict("lev_item_test");

	fixture.merge_leveled_list(entry);

	REQUIRE(fixture.scan.merge_record_count() == 1);
	auto items = extract_items_from_content(fixture.merged_content());

	REQUIRE(items.size() == 3);
	REQUIRE(items[0] == std::pair<std::string, uint16_t> { "iron_sword", 1 });
	REQUIRE(items[1] == std::pair<std::string, uint16_t> { "steel_dagger", 3 });
	REQUIRE(items[2] == std::pair<std::string, uint16_t> { "glass_blade", 5 });
}

TEST_CASE("plugin_scan_t::merge_leveled_list, deduplication by item and level", "[i]")
{
	auto master_levi = make_levi_record(
	    "lev_item_dup",
	    1,
	    0,
	    {
	        { "iron_sword", 1 },
	        { "steel_sword", 5 },
	    });

	auto plugin_levi = make_levi_record(
	    "lev_item_dup",
	    1,
	    0,
	    {
	        { "iron_sword", 1 },
	        { "steel_sword", 5 },
	        { "daedric_axe", 10 },
	    });

	merge_test_fixture_t fixture(master_levi, plugin_levi);
	auto entry = fixture.make_conflict("lev_item_dup");

	fixture.merge_leveled_list(entry);

	REQUIRE(fixture.scan.merge_record_count() == 1);
	auto items = extract_items_from_content(fixture.merged_content());

	REQUIRE(items.size() == 3);
	REQUIRE(items[0] == std::pair<std::string, uint16_t> { "iron_sword", 1 });
	REQUIRE(items[1] == std::pair<std::string, uint16_t> { "steel_sword", 5 });
	REQUIRE(items[2] == std::pair<std::string, uint16_t> { "daedric_axe", 10 });
}

TEST_CASE("plugin_scan_t::merge_leveled_list, header from winner", "[i]")
{
	uint32_t master_flags = 0x02;
	uint8_t master_chance = 25;

	auto master_levi = make_levi_record(
	    "lev_header_test",
	    master_flags,
	    master_chance,
	    {
	        { "iron_sword", 1 },
	    });

	uint32_t plugin_flags = 0x01;
	uint8_t plugin_chance = 50;

	auto plugin_levi = make_levi_record(
	    "lev_header_test",
	    plugin_flags,
	    plugin_chance,
	    {
	        { "iron_sword", 1 },
	        { "glass_blade", 5 },
	    });

	merge_test_fixture_t fixture(master_levi, plugin_levi);
	auto entry = fixture.make_conflict("lev_header_test");

	fixture.merge_leveled_list(entry);

	REQUIRE(fixture.scan.merge_record_count() == 1);

	auto merged_data = find_sub_record(fixture.merged_content(), "DATA");
	auto merged_nnam = find_sub_record(fixture.merged_content(), "NNAM");
	auto plugin_data = find_sub_record(plugin_levi, "DATA");
	auto plugin_nnam = find_sub_record(plugin_levi, "NNAM");

	REQUIRE(merged_data == plugin_data);
	REQUIRE(merged_nnam == plugin_nnam);
}

TEST_CASE("plugin_scan_t::merge_leveled_list, same item different levels are distinct", "[i]")
{
	auto master_levi = make_levi_record(
	    "lev_item_levels",
	    1,
	    0,
	    {
	        { "iron_sword", 1 },
	    });

	auto plugin_levi = make_levi_record(
	    "lev_item_levels",
	    1,
	    0,
	    {
	        { "iron_sword", 1 },
	        { "iron_sword", 5 },
	        { "iron_sword", 10 },
	    });

	merge_test_fixture_t fixture(master_levi, plugin_levi);
	auto entry = fixture.make_conflict("lev_item_levels");

	fixture.merge_leveled_list(entry);

	REQUIRE(fixture.scan.merge_record_count() == 1);
	auto items = extract_items_from_content(fixture.merged_content());

	REQUIRE(items.size() == 3);
	REQUIRE(items[0] == std::pair<std::string, uint16_t> { "iron_sword", 1 });
	REQUIRE(items[1] == std::pair<std::string, uint16_t> { "iron_sword", 5 });
	REQUIRE(items[2] == std::pair<std::string, uint16_t> { "iron_sword", 10 });
}

static std::string null_terminated(const std::string & text)
{
	return text + std::string(1, '\0');
}

static std::string make_dial_record(const std::string & topic_name, uint8_t dial_type = 0)
{
	std::string data_content(1, static_cast<char>(dial_type));
	std::string subs;
	subs += make_sub_record("NAME", null_terminated(topic_name));
	subs += make_sub_record("DATA", data_content);
	return make_record("DIAL", subs);
}

static std::string make_info_record(
    const std::string & info_id,
    const std::string & response_text,
    const std::string & prev_id = "",
    const std::string & next_id = "")
{
	std::string subs;
	subs += make_sub_record("INAM", null_terminated(info_id));
	subs += make_sub_record("PNAM", null_terminated(prev_id));
	subs += make_sub_record("NNAM", null_terminated(next_id));
	subs += make_sub_record("NAME", null_terminated(response_text));
	return make_record("INFO", subs);
}

static std::string make_plugin(const std::vector<std::string> & records)
{
	std::string result = make_tes3_record();
	for (const auto & rec : records)
		result += rec;
	return result;
}

struct merge_dialogue_fixture_t
{
	plugin_scan_t scan;
	std::vector<std::string> temp_files;

	void add_plugin(const std::string & filename, const std::string & content)
	{
		const auto path = get_temp_path(filename);
		write_binary_file(path, content);
		scan.load_plugin(path);
		temp_files.push_back(path);
	}

	void setup_merge()
	{
		scan.set_merge_plugin("Merged.esp");
		scan.rebuild_conflicts();
	}

	void merge_dialogue(const conflict_entry_t & entry)
	{
		const auto & winning_ver = entry.versions.back();
		std::string winning_dial = scan.read_record_content(winning_ver.plugin_idx, winning_ver.record_index);
		scan.copy_record_to_merge_raw("DIAL", entry.record_id, winning_dial);

		std::vector<std::string> merged_info_ids;
		std::map<std::string, std::string> info_contents;

		for (const auto & ver : entry.versions)
		{
			if (scan.is_merge_plugin(ver.plugin_idx))
				continue;

			const auto & plugin_entries = scan.index(ver.plugin_idx).entries();
			for (size_t ei = ver.record_index + 1; ei < plugin_entries.size(); ++ei)
			{
				if (plugin_entries[ei].rec_type != "INFO")
					break;

				if (plugin_entries[ei].dial_name != entry.record_id)
					break;

				const auto & info_id = plugin_entries[ei].record_id;
				std::string content = scan.read_record_content(ver.plugin_idx, plugin_entries[ei].record_index);

				if (info_contents.find(info_id) == info_contents.end())
					merged_info_ids.push_back(info_id);

				info_contents[info_id] = content;
			}
		}

		for (const auto & info_id : merged_info_ids)
			scan.copy_record_to_merge_raw("INFO", info_id, info_contents[info_id]);
	}

	~merge_dialogue_fixture_t()
	{
		for (const auto & path : temp_files)
			std::filesystem::remove(path);
	}
};

TEST_CASE("plugin_scan_t::merge_dialogue, DIAL content from last plugin", "[i]")
{
	merge_dialogue_fixture_t fixture;

	const auto plugin_a = make_plugin(
	    {
	        make_dial_record("greeting", 0),
	        make_info_record("info1", "hello from A"),
	    });

	const auto plugin_b = make_plugin(
	    {
	        make_dial_record("greeting", 2),
	        make_info_record("info1", "hello from B"),
	    });

	fixture.add_plugin("yampt_test_dial_a.esp", plugin_a);
	fixture.add_plugin("yampt_test_dial_b.esp", plugin_b);
	fixture.setup_merge();

	const auto * entry = fixture.scan.find("DIAL", "greeting");
	REQUIRE(entry != nullptr);

	fixture.merge_dialogue(*entry);

	REQUIRE(fixture.scan.merge_record_count() >= 1);

	const auto & dial_content = fixture.scan.merge_record_content(0);
	const auto expected_dial_b = make_dial_record("greeting", 2);
	REQUIRE(dial_content == expected_dial_b);
}

TEST_CASE("plugin_scan_t::merge_dialogue, INFO ordering preserves first-seen position", "[i]")
{
	merge_dialogue_fixture_t fixture;

	const auto plugin_a = make_plugin(
	    {
	        make_dial_record("topic"),
	        make_info_record("alpha", "response alpha"),
	        make_info_record("beta", "response beta"),
	    });

	const auto plugin_b = make_plugin(
	    {
	        make_dial_record("topic"),
	        make_info_record("beta", "response beta v2"),
	        make_info_record("gamma", "response gamma"),
	    });

	fixture.add_plugin("yampt_test_order_a.esp", plugin_a);
	fixture.add_plugin("yampt_test_order_b.esp", plugin_b);
	fixture.setup_merge();

	const auto * entry = fixture.scan.find("DIAL", "topic");
	REQUIRE(entry != nullptr);

	fixture.merge_dialogue(*entry);

	REQUIRE(fixture.scan.merge_record_count() == 4);

	const auto & info1_content = fixture.scan.merge_record_content(1);
	const auto & info2_content = fixture.scan.merge_record_content(2);
	const auto & info3_content = fixture.scan.merge_record_content(3);

	REQUIRE(info1_content.find("alpha") != std::string::npos);
	REQUIRE(info2_content.find("beta") != std::string::npos);
	REQUIRE(info3_content.find("gamma") != std::string::npos);
}

TEST_CASE("plugin_scan_t::merge_dialogue, all distinct INFO IDs included", "[i]")
{
	merge_dialogue_fixture_t fixture;

	const auto plugin_a = make_plugin(
	    {
	        make_dial_record("quest"),
	        make_info_record("info_x", "x text"),
	        make_info_record("info_y", "y text"),
	    });

	const auto plugin_b = make_plugin(
	    {
	        make_dial_record("quest"),
	        make_info_record("info_z", "z text"),
	    });

	const auto plugin_c = make_plugin(
	    {
	        make_dial_record("quest"),
	        make_info_record("info_w", "w text"),
	    });

	fixture.add_plugin("yampt_test_all_a.esp", plugin_a);
	fixture.add_plugin("yampt_test_all_b.esp", plugin_b);
	fixture.add_plugin("yampt_test_all_c.esp", plugin_c);
	fixture.setup_merge();

	const auto * entry = fixture.scan.find("DIAL", "quest");
	REQUIRE(entry != nullptr);

	fixture.merge_dialogue(*entry);

	REQUIRE(fixture.scan.merge_record_count() == 5);

	const auto & content1 = fixture.scan.merge_record_content(1);
	const auto & content2 = fixture.scan.merge_record_content(2);
	const auto & content3 = fixture.scan.merge_record_content(3);
	const auto & content4 = fixture.scan.merge_record_content(4);

	REQUIRE(content1.find("info_x") != std::string::npos);
	REQUIRE(content2.find("info_y") != std::string::npos);
	REQUIRE(content3.find("info_z") != std::string::npos);
	REQUIRE(content4.find("info_w") != std::string::npos);
}

TEST_CASE("plugin_scan_t::merge_dialogue, INFO content last-wins for duplicates", "[i]")
{
	merge_dialogue_fixture_t fixture;

	const auto plugin_a = make_plugin(
	    {
	        make_dial_record("rumors"),
	        make_info_record("shared", "old rumor text"),
	    });

	const auto plugin_b = make_plugin(
	    {
	        make_dial_record("rumors"),
	        make_info_record("shared", "updated rumor text"),
	    });

	fixture.add_plugin("yampt_test_lastwins_a.esp", plugin_a);
	fixture.add_plugin("yampt_test_lastwins_b.esp", plugin_b);
	fixture.setup_merge();

	const auto * entry = fixture.scan.find("DIAL", "rumors");
	REQUIRE(entry != nullptr);

	fixture.merge_dialogue(*entry);

	REQUIRE(fixture.scan.merge_record_count() == 2);

	const auto & info_content = fixture.scan.merge_record_content(1);
	REQUIRE(info_content.find("updated rumor text") != std::string::npos);
	REQUIRE(info_content.find("old rumor text") == std::string::npos);
}

TEST_CASE("plugin_scan_t::merge_dialogue, orphan PNAM/NNAM references ignored", "[i]")
{
	merge_dialogue_fixture_t fixture;

	const auto plugin_a = make_plugin(
	    {
	        make_dial_record("lore"),
	        make_info_record("real1", "lore text one", "", "ghost_id"),
	        make_info_record("real2", "lore text two", "ghost_id", ""),
	    });

	fixture.add_plugin("yampt_test_orphan_a.esp", plugin_a);
	fixture.setup_merge();

	const auto * entry = fixture.scan.find("DIAL", "lore");
	REQUIRE(entry != nullptr);

	fixture.merge_dialogue(*entry);

	REQUIRE(fixture.scan.merge_record_count() == 3);

	const auto & content1 = fixture.scan.merge_record_content(1);
	const auto & content2 = fixture.scan.merge_record_content(2);

	REQUIRE(content1.find("lore text one") != std::string::npos);
	REQUIRE(content2.find("lore text two") != std::string::npos);
}
