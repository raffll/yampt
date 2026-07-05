#include <catch2/catch_all.hpp>
#include <decoder/content_alignment.hpp>
#include <cstring>
#include <string>

static sub_record_view_t make_view(const std::string & type, const char * data, size_t size)
{
	sub_record_view_t view;
	view.type = type;
	view.data = data;
	view.size = size;
	return view;
}

TEST_CASE("content_alignment_t::align, single anchor no trailing", "[u]")
{
	const char npcs_a[] = "spell_a\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	const char npcs_b[] = "spell_b\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

	std::vector<std::vector<sub_record_view_t>> all_subs(2);
	all_subs[0].push_back(make_view("NPCS", npcs_a, 32));
	all_subs[0].push_back(make_view("NPCS", npcs_b, 32));
	all_subs[1].push_back(make_view("NPCS", npcs_b, 32));

	alignment_rule_t rule;
	rule.anchor_type = "NPCS";
	rule.anchor_size = 32;
	rule.key_source = alignment_rule_t::key_from_t::anchor;

	std::vector<sub_slot_t> slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> indices(2);

	content_alignment_t::align(all_subs, 2, { rule }, slots, indices);

	REQUIRE(slots.size() == 2);
	REQUIRE(indices[0]["NPCS"].size() == 2);
	REQUIRE(indices[1]["NPCS"].size() == 2);

	REQUIRE(indices[0]["NPCS"][0] == 0);
	REQUIRE(indices[0]["NPCS"][1] == 1);
	REQUIRE(indices[1]["NPCS"][0] == SIZE_MAX);
	REQUIRE(indices[1]["NPCS"][1] == 0);
}

TEST_CASE("content_alignment_t::align, anchor with trailing", "[u]")
{
	const char indx_a[] = "\x04";
	const char indx_b[] = "\x05";
	const char bnam_a[] = "mesh_a";
	const char bnam_b[] = "mesh_b";

	std::vector<std::vector<sub_record_view_t>> all_subs(2);
	all_subs[0].push_back(make_view("INDX", indx_a, 1));
	all_subs[0].push_back(make_view("BNAM", bnam_a, 6));
	all_subs[1].push_back(make_view("INDX", indx_a, 1));
	all_subs[1].push_back(make_view("BNAM", bnam_b, 6));
	all_subs[1].push_back(make_view("INDX", indx_b, 1));
	all_subs[1].push_back(make_view("BNAM", bnam_b, 6));

	alignment_rule_t rule;
	rule.anchor_type = "INDX";
	rule.anchor_size = 0;
	rule.trailing_types = { "BNAM", "CNAM" };
	rule.key_source = alignment_rule_t::key_from_t::anchor;

	std::vector<sub_slot_t> slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> indices(2);

	content_alignment_t::align(all_subs, 2, { rule }, slots, indices);

	REQUIRE(indices[0]["INDX"].size() == 2);
	REQUIRE(indices[1]["INDX"].size() == 2);

	REQUIRE(indices[0]["INDX"][0] == 0);
	REQUIRE(indices[0]["INDX"][1] == SIZE_MAX);
	REQUIRE(indices[1]["INDX"][0] == 0);
	REQUIRE(indices[1]["INDX"][1] == 2);

	REQUIRE(indices[0]["BNAM"][0] == 1);
	REQUIRE(indices[0]["BNAM"][1] == SIZE_MAX);
	REQUIRE(indices[1]["BNAM"][0] == 1);
	REQUIRE(indices[1]["BNAM"][1] == 3);
}

TEST_CASE("content_alignment_t::align, non-excluded sub-records preserved", "[u]")
{
	const char indx_a[] = "\x04";
	const char bnam_a[] = "mesh_a";
	const char enam_a[] = "ench_id";

	std::vector<std::vector<sub_record_view_t>> all_subs(1);
	all_subs[0].push_back(make_view("ENAM", enam_a, 7));
	all_subs[0].push_back(make_view("INDX", indx_a, 1));
	all_subs[0].push_back(make_view("BNAM", bnam_a, 6));

	alignment_rule_t rule;
	rule.anchor_type = "INDX";
	rule.anchor_size = 0;
	rule.trailing_types = { "BNAM", "CNAM" };
	rule.key_source = alignment_rule_t::key_from_t::anchor;

	std::vector<sub_slot_t> slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> indices(1);

	content_alignment_t::align(all_subs, 1, { rule }, slots, indices);

	bool has_enam_slot = false;
	for (const auto & slot : slots)
	{
		if (slot.type == "ENAM")
			has_enam_slot = true;
	}
	REQUIRE(has_enam_slot);
	REQUIRE(indices[0]["ENAM"].size() == 1);
	REQUIRE(indices[0]["ENAM"][0] == 0);
}

TEST_CASE("content_alignment_t::align, multiple rules excludes all anchors", "[u]")
{
	const char npco_data[36] = {};
	const char npcs_data[32] = {};

	std::vector<std::vector<sub_record_view_t>> all_subs(1);
	all_subs[0].push_back(make_view("NPCO", npco_data, 36));
	all_subs[0].push_back(make_view("NPCS", npcs_data, 32));

	alignment_rule_t npco_rule;
	npco_rule.anchor_type = "NPCO";
	npco_rule.anchor_size = 36;
	npco_rule.key_source = alignment_rule_t::key_from_t::anchor;

	alignment_rule_t npcs_rule;
	npcs_rule.anchor_type = "NPCS";
	npcs_rule.anchor_size = 32;
	npcs_rule.key_source = alignment_rule_t::key_from_t::anchor;

	std::vector<sub_slot_t> slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> indices(1);

	content_alignment_t::align(all_subs, 1, { npco_rule, npcs_rule }, slots, indices);

	int npco_count = 0;
	int npcs_count = 0;
	for (const auto & slot : slots)
	{
		if (slot.type == "NPCO")
			++npco_count;

		if (slot.type == "NPCS")
			++npcs_count;
	}

	REQUIRE(npco_count == 1);
	REQUIRE(npcs_count == 1);
}

TEST_CASE("content_alignment_t::align, key_from_t::offset extracts partial content", "[u]")
{
	char npco_a[36] = {};
	std::memcpy(npco_a + 4, "iron_sword", 10);

	char npco_b[36] = {};
	std::memcpy(npco_b + 4, "iron_sword", 10);

	std::vector<std::vector<sub_record_view_t>> all_subs(2);
	all_subs[0].push_back(make_view("NPCO", npco_a, 36));
	all_subs[1].push_back(make_view("NPCO", npco_b, 36));

	alignment_rule_t rule;
	rule.anchor_type = "NPCO";
	rule.anchor_size = 36;
	rule.key_source = alignment_rule_t::key_from_t::offset;
	rule.key_offset = 4;
	rule.key_length = 32;

	std::vector<sub_slot_t> slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> indices(2);

	content_alignment_t::align(all_subs, 2, { rule }, slots, indices);

	REQUIRE(indices[0]["NPCO"].size() == 1);
	REQUIRE(indices[1]["NPCO"].size() == 1);
	REQUIRE(indices[0]["NPCO"][0] == 0);
	REQUIRE(indices[1]["NPCO"][0] == 0);
}

TEST_CASE("content_alignment_t::build_occurrence_based, simple ordering", "[u]")
{
	const char data_a[] = "a";
	const char data_b[] = "b";

	std::vector<std::vector<sub_record_view_t>> all_subs(2);
	all_subs[0].push_back(make_view("NAME", data_a, 1));
	all_subs[0].push_back(make_view("FNAM", data_a, 1));
	all_subs[1].push_back(make_view("NAME", data_b, 1));
	all_subs[1].push_back(make_view("FNAM", data_b, 1));

	std::vector<sub_slot_t> slots;
	std::vector<std::unordered_map<std::string, std::vector<size_t>>> indices(2);

	content_alignment_t::build_occurrence_based(all_subs, 2, slots, indices);

	REQUIRE(slots.size() == 2);
	REQUIRE(slots[0].type == "NAME");
	REQUIRE(slots[1].type == "FNAM");
	REQUIRE(indices[0]["NAME"][0] == 0);
	REQUIRE(indices[1]["NAME"][0] == 0);
}
