#include <catch2/catch_all.hpp>
#include "../yampt/plugin_scan/conflict_slots.hpp"
#include <string>
#include <cstring>

static std::string make_sub_record(const std::string & type, const std::string & data)
{
	std::string result;
	result += type;
	uint32_t size = static_cast<uint32_t>(data.size());
	result.append(reinterpret_cast<const char *>(&size), 4);
	result += data;
	return result;
}

static std::string make_record_content(const std::vector<std::pair<std::string, std::string>> & sub_records)
{
	std::string header(16, '\0');
	for (const auto & [type, data] : sub_records)
		header += make_sub_record(type, data);
	return header;
}

TEST_CASE("build_generic_slots, two versions identical sub-records", "[u]")
{
	std::string v1 = make_record_content({{"NAME", "abc"}, {"DATA", "1234"}});
	std::string v2 = make_record_content({{"NAME", "abc"}, {"DATA", "1234"}});

	auto result = build_conflict_slots("ARMO", {v1, v2}, {false, false});

	REQUIRE(result.aligned.size() == 2);
	REQUIRE(result.aligned[0].key.type == "NAME");
	REQUIRE(result.aligned[0].key.occurrence == 0);
	REQUIRE(result.aligned[0].indices[0] != SIZE_MAX);
	REQUIRE(result.aligned[0].indices[1] != SIZE_MAX);
	REQUIRE(result.aligned[1].key.type == "DATA");
	REQUIRE(result.aligned[1].key.occurrence == 0);
	REQUIRE(result.aligned[1].indices[0] != SIZE_MAX);
	REQUIRE(result.aligned[1].indices[1] != SIZE_MAX);
}

TEST_CASE("build_generic_slots, version 2 lacks a sub-record type", "[u]")
{
	std::string v1 = make_record_content({{"NAME", "abc"}, {"DATA", "1234"}, {"FNAM", "xyz"}});
	std::string v2 = make_record_content({{"NAME", "abc"}, {"DATA", "1234"}});

	auto result = build_conflict_slots("ARMO", {v1, v2}, {false, false});

	REQUIRE(result.aligned.size() == 3);
	REQUIRE(result.aligned[2].key.type == "FNAM");
	REQUIRE(result.aligned[2].key.occurrence == 0);
	REQUIRE(result.aligned[2].indices[0] != SIZE_MAX);
	REQUIRE(result.aligned[2].indices[1] == SIZE_MAX);
}

TEST_CASE("build_generic_slots, multiple occurrences aligned by occurrence order", "[u]")
{
	std::string v1 = make_record_content({{"NAME", "a"}, {"NAME", "b"}, {"NAME", "c"}});
	std::string v2 = make_record_content({{"NAME", "x"}, {"NAME", "y"}});

	auto result = build_conflict_slots("ARMO", {v1, v2}, {false, false});

	REQUIRE(result.aligned.size() == 3);
	REQUIRE(result.aligned[0].key.type == "NAME");
	REQUIRE(result.aligned[0].key.occurrence == 0);
	REQUIRE(result.aligned[0].indices[0] == 0);
	REQUIRE(result.aligned[0].indices[1] == 0);

	REQUIRE(result.aligned[1].key.type == "NAME");
	REQUIRE(result.aligned[1].key.occurrence == 1);
	REQUIRE(result.aligned[1].indices[0] == 1);
	REQUIRE(result.aligned[1].indices[1] == 1);

	REQUIRE(result.aligned[2].key.type == "NAME");
	REQUIRE(result.aligned[2].key.occurrence == 2);
	REQUIRE(result.aligned[2].indices[0] == 2);
	REQUIRE(result.aligned[2].indices[1] == SIZE_MAX);
}

TEST_CASE("build_generic_slots, empty content yields empty parsed and all SIZE_MAX", "[u]")
{
	std::string v1(10, '\0');
	std::string v2 = make_record_content({{"NAME", "abc"}});

	auto result = build_conflict_slots("ARMO", {v1, v2}, {false, false});

	REQUIRE(result.parsed[0].empty());
	REQUIRE(result.aligned.size() == 1);
	REQUIRE(result.aligned[0].indices[0] == SIZE_MAX);
	REQUIRE(result.aligned[0].indices[1] == 0);
}

TEST_CASE("build_leveled_list_slots, LEVI with 3 items version 2 lacks middle", "[u]")
{
	auto intv2 = [](uint16_t level) {
		std::string data(2, '\0');
		std::memcpy(data.data(), &level, 2);
		return data;
	};

	std::string v1 = make_record_content({
		{"NAME", "list_id"},
		{"DATA", "\x01\x00\x00\x00"},
		{"NNAM", "\x03"},
		{"INTV", intv2(1)}, {"INAM", "item_a"},
		{"INTV", intv2(2)}, {"INAM", "item_b"},
		{"INTV", intv2(3)}, {"INAM", "item_c"}
	});

	std::string v2 = make_record_content({
		{"NAME", "list_id"},
		{"DATA", "\x01\x00\x00\x00"},
		{"NNAM", "\x02"},
		{"INTV", intv2(1)}, {"INAM", "item_a"},
		{"INTV", intv2(3)}, {"INAM", "item_c"}
	});

	auto result = build_conflict_slots("LEVI", {v1, v2}, {false, false});

	bool found_b_intv = false;
	for (const auto & slot : result.aligned)
	{
		if (slot.key.type == "INAM")
		{
			size_t idx0 = slot.indices[0];
			if (idx0 != SIZE_MAX)
			{
				std::string id(result.parsed[0][idx0].data, result.parsed[0][idx0].size);
				if (!id.empty() && id.back() == '\0') id.pop_back();
				if (id == "item_b")
				{
					found_b_intv = true;
					REQUIRE(slot.indices[1] == SIZE_MAX);
				}
			}
		}
	}
	REQUIRE(found_b_intv);
}

TEST_CASE("build_leveled_list_slots, header INTV size != 2 stays in headers", "[u]")
{
	std::string v1 = make_record_content({
		{"NAME", "list_id"},
		{"DATA", "\x01\x00\x00\x00"},
		{"INTV", "\x05\x00\x00\x00"},
		{"NNAM", "\x01"},
		{"INTV", std::string(2, '\x01')}, {"INAM", "item_a"}
	});

	auto result = build_conflict_slots("LEVI", {v1}, {false});

	bool found_header_intv = false;
	for (const auto & slot : result.aligned)
	{
		if (slot.key.type == "INTV" && slot.indices[0] != SIZE_MAX)
		{
			const auto & sv = result.parsed[0][slot.indices[0]];
			if (sv.size == 4)
				found_header_intv = true;
		}
	}
	REQUIRE(found_header_intv);
}

TEST_CASE("build_container_slots, NPCO item ID extracted from bytes 4..36", "[u]")
{
	std::string npco_data(36, '\0');
	uint32_t count = 5;
	std::memcpy(npco_data.data(), &count, 4);
	std::memcpy(npco_data.data() + 4, "iron_sword", 10);

	std::string v1 = make_record_content({
		{"NAME", "npc_id"},
		{"NPCO", npco_data}
	});
	std::string v2 = make_record_content({
		{"NAME", "npc_id"}
	});

	auto result = build_conflict_slots("NPC_", {v1, v2}, {false, false});

	bool found_npco = false;
	for (const auto & slot : result.aligned)
	{
		if (slot.key.type == "NPCO")
		{
			REQUIRE(slot.indices[0] != SIZE_MAX);
			REQUIRE(slot.indices[1] == SIZE_MAX);
			found_npco = true;
		}
	}
	REQUIRE(found_npco);
}

TEST_CASE("build_cell_slots, header ends before first FRMR", "[u]")
{
	uint32_t obj1 = 100;
	std::string frmr_data(4, '\0');
	std::memcpy(frmr_data.data(), &obj1, 4);

	std::string v1 = make_record_content({
		{"NAME", "cell_name"},
		{"DATA", std::string(12, '\0')},
		{"FRMR", frmr_data},
		{"NAME", "object_id"},
		{"DATA", std::string(24, '\0')}
	});

	auto result = build_conflict_slots("CELL", {v1}, {false});

	REQUIRE(result.aligned.size() >= 2);
	REQUIRE(result.aligned[0].key.type == "NAME");
	REQUIRE(result.aligned[1].key.type == "DATA");

	bool found_frmr = false;
	for (const auto & slot : result.aligned)
	{
		if (slot.key.type == "FRMR")
		{
			found_frmr = true;
			break;
		}
	}
	REQUIRE(found_frmr);
}

TEST_CASE("build_conflict_slots, pointer stability after construction", "[u]")
{
	std::string v1 = make_record_content({{"NAME", "test_data_123456789"}});
	std::string v2 = make_record_content({{"NAME", "other_value_abcdefgh"}});

	auto result = build_conflict_slots("ARMO", {v1, v2}, {false, false});

	REQUIRE(result.parsed[0].size() == 1);
	REQUIRE(result.parsed[1].size() == 1);

	const char * ptr0 = result.parsed[0][0].data;
	const char * ptr1 = result.parsed[1][0].data;

	REQUIRE(ptr0 >= result.contents[0].data());
	REQUIRE(ptr0 < result.contents[0].data() + result.contents[0].size());
	REQUIRE(ptr1 >= result.contents[1].data());
	REQUIRE(ptr1 < result.contents[1].data() + result.contents[1].size());

	REQUIRE(std::string(ptr0, result.parsed[0][0].size) == "test_data_123456789");
	REQUIRE(std::string(ptr1, result.parsed[1][0].size) == "other_value_abcdefgh");
}

TEST_CASE("build_conflict_slots, is_deleted flags stored correctly", "[u]")
{
	std::string v1 = make_record_content({{"NAME", "abc"}});
	std::string v2 = make_record_content({{"NAME", "abc"}});

	auto result = build_conflict_slots("ARMO", {v1, v2}, {false, true});

	REQUIRE(result.is_deleted.size() == 2);
	REQUIRE(result.is_deleted[0] == false);
	REQUIRE(result.is_deleted[1] == true);
}

TEST_CASE("build_fact_slots, faction paired by ANAM content", "[u]")
{
	auto intv4 = [](uint32_t val) {
		std::string data(4, '\0');
		std::memcpy(data.data(), &val, 4);
		return data;
	};

	std::string v1 = make_record_content({
		{"NAME", "faction_id"},
		{"FNAM", "Faction Name"},
		{"INTV", intv4(0)}, {"ANAM", "Guild_A"},
		{"INTV", intv4(1)}, {"ANAM", "Guild_B"}
	});

	std::string v2 = make_record_content({
		{"NAME", "faction_id"},
		{"FNAM", "Faction Name"},
		{"INTV", intv4(0)}, {"ANAM", "Guild_A"},
		{"INTV", intv4(2)}, {"ANAM", "Guild_C"}
	});

	auto result = build_conflict_slots("FACT", {v1, v2}, {false, false});

	int anam_slots = 0;
	bool found_guild_b = false;
	bool found_guild_c = false;
	for (const auto & slot : result.aligned)
	{
		if (slot.key.type != "ANAM")
			continue;

		++anam_slots;

		if (slot.indices[0] != SIZE_MAX)
		{
			const auto & sv = result.parsed[0][slot.indices[0]];
			std::string name(sv.data, sv.size);
			if (!name.empty() && name.back() == '\0') name.pop_back();
			if (name == "Guild_B")
			{
				found_guild_b = true;
				REQUIRE(slot.indices[1] == SIZE_MAX);
			}
		}

		if (slot.indices[1] != SIZE_MAX)
		{
			const auto & sv = result.parsed[1][slot.indices[1]];
			std::string name(sv.data, sv.size);
			if (!name.empty() && name.back() == '\0') name.pop_back();
			if (name == "Guild_C")
			{
				found_guild_c = true;
				REQUIRE(slot.indices[0] == SIZE_MAX);
			}
		}
	}

	REQUIRE(anam_slots == 3);
	REQUIRE(found_guild_b);
	REQUIRE(found_guild_c);
}
