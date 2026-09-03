#include <catch2/catch_all.hpp>
#include <scanner/merge_patch_ops.hpp>
#include <cstring>
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

static std::string make_uint32(uint32_t value)
{
	std::string result(4, '\0');
	std::memcpy(result.data(), &value, 4);
	return result;
}

static std::string make_data_12(uint32_t flags, int32_t grid_x, int32_t grid_y)
{
	std::string result(12, '\0');
	std::memcpy(result.data(), &flags, 4);
	std::memcpy(result.data() + 4, &grid_x, 4);
	std::memcpy(result.data() + 8, &grid_y, 4);
	return result;
}

TEST_CASE("merge_patch_ops_t::patch_sub_record, replaces existing sub-record", "[u]")
{
	auto merge = make_record(
	    "WEAP",
	    make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("Old Name")) +
	        make_sub("ENAM", make_string("old_ench")));

	auto source = make_record(
	    "WEAP",
	    make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("New Name")) +
	        make_sub("ENAM", make_string("new_ench")));

	auto result = merge_patch_ops_t::patch_sub_record(merge, source, "FNAM", 1);

	REQUIRE(result.success);
	REQUIRE(result.content.find("New Name") != std::string::npos);
	REQUIRE(result.content.find("Old Name") == std::string::npos);
	REQUIRE(result.content.find("old_ench") != std::string::npos);
}

TEST_CASE("merge_patch_ops_t::patch_sub_record, appends if not in merge", "[u]")
{
	auto merge = make_record("BSGN", make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("Sign")));

	auto source = make_record(
	    "BSGN",
	    make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("Sign")) +
	        make_sub("NPCS", make_string("ability")));

	auto result = merge_patch_ops_t::patch_sub_record(merge, source, "NPCS", 2);

	REQUIRE(result.success);
	REQUIRE(result.content.find("ability") != std::string::npos);
	REQUIRE(result.content.find("Sign") != std::string::npos);
}

TEST_CASE("merge_patch_ops_t::patch_sub_record, second occurrence patched", "[u]")
{
	auto merge = make_record(
	    "BSGN",
	    make_sub("NAME", make_string("id")) + make_sub("NPCS", make_string("first")) +
	        make_sub("NPCS", make_string("second_old")));

	auto source = make_record(
	    "BSGN",
	    make_sub("NAME", make_string("id")) + make_sub("NPCS", make_string("first")) +
	        make_sub("NPCS", make_string("second_new")));

	auto result = merge_patch_ops_t::patch_sub_record(merge, source, "NPCS", 2);

	REQUIRE(result.success);
	REQUIRE(result.content.find("first") != std::string::npos);
	REQUIRE(result.content.find("second_new") != std::string::npos);
}

TEST_CASE("merge_patch_ops_t::patch_sub_record, binary_idx out of range fails", "[u]")
{
	auto merge = make_record("NPC_", make_sub("NAME", make_string("id")));
	auto source = make_record("NPC_", make_sub("NAME", make_string("id")));

	auto result = merge_patch_ops_t::patch_sub_record(merge, source, "FNAM", 5);

	REQUIRE_FALSE(result.success);
}

TEST_CASE("merge_patch_ops_t::patch_group, patches multiple sub-records", "[u]")
{
	auto merge = make_record(
	    "ARMO",
	    make_sub("NAME", make_string("id")) + make_sub("INDX", make_uint32(0)) +
	        make_sub("BNAM", make_string("old_male")) + make_sub("CNAM", make_string("old_female")));

	auto source = make_record(
	    "ARMO",
	    make_sub("NAME", make_string("id")) + make_sub("INDX", make_uint32(0)) +
	        make_sub("BNAM", make_string("new_male")) + make_sub("CNAM", make_string("new_female")));

	std::vector<std::pair<std::string, int>> indices = {
		{ "INDX", 1 },
		{ "BNAM", 2 },
		{ "CNAM", 3 },
	};

	auto result = merge_patch_ops_t::patch_group(merge, source, indices);

	REQUIRE(result.success);
	REQUIRE(result.content.find("new_male") != std::string::npos);
	REQUIRE(result.content.find("new_female") != std::string::npos);
	REQUIRE(result.content.find("old_male") == std::string::npos);
	REQUIRE(result.content.find("old_female") == std::string::npos);
}

TEST_CASE("merge_patch_ops_t::patch_group, appends missing sub-records", "[u]")
{
	auto merge = make_record("ARMO", make_sub("NAME", make_string("id")) + make_sub("INDX", make_uint32(0)));

	auto source = make_record(
	    "ARMO",
	    make_sub("NAME", make_string("id")) + make_sub("INDX", make_uint32(0)) +
	        make_sub("BNAM", make_string("male_part")) + make_sub("CNAM", make_string("female_part")));

	std::vector<std::pair<std::string, int>> indices = {
		{ "INDX", 1 },
		{ "BNAM", 2 },
		{ "CNAM", 3 },
	};

	auto result = merge_patch_ops_t::patch_group(merge, source, indices);

	REQUIRE(result.success);
	REQUIRE(result.content.find("male_part") != std::string::npos);
	REQUIRE(result.content.find("female_part") != std::string::npos);
}

TEST_CASE("merge_patch_ops_t::patch_group, skips invalid binary indices", "[u]")
{
	auto merge = make_record("ARMO", make_sub("NAME", make_string("id")) + make_sub("INDX", make_uint32(0)));

	auto source = make_record("ARMO", make_sub("NAME", make_string("id")) + make_sub("INDX", make_uint32(0)));

	std::vector<std::pair<std::string, int>> indices = {
		{ "INDX", 1 },
		{ "BNAM", 99 },
	};

	auto result = merge_patch_ops_t::patch_group(merge, source, indices);

	REQUIRE(result.success);
}

TEST_CASE("merge_patch_ops_t::patch_field, patches single field in CELL DATA", "[u]")
{
	auto merge_data = make_data_12(0x01, 5, 10);
	auto source_data = make_data_12(0x01, 99, 10);

	auto merge = make_record("CELL", make_sub("NAME", make_string("cell")) + make_sub("DATA", merge_data));

	auto source = make_record("CELL", make_sub("NAME", make_string("cell")) + make_sub("DATA", source_data));

	auto result = merge_patch_ops_t::patch_field(merge, source, "CELL", "DATA", 12, 1, 1);

	REQUIRE(result.success);

	auto patched_subs = sub_record_merge_t::parse_sub_records(result.content);
	REQUIRE(patched_subs.size() == 2);

	int32_t patched_grid_x = 0;
	std::memcpy(&patched_grid_x, patched_subs[1].data.data() + 4, 4);
	REQUIRE(patched_grid_x == 99);

	int32_t patched_grid_y = 0;
	std::memcpy(&patched_grid_y, patched_subs[1].data.data() + 8, 4);
	REQUIRE(patched_grid_y == 10);

	uint32_t patched_flags = 0;
	std::memcpy(&patched_flags, patched_subs[1].data.data(), 4);
	REQUIRE(patched_flags == 0x01);
}

TEST_CASE("merge_patch_ops_t::patch_field, patches flags only", "[u]")
{
	auto merge_data = make_data_12(0x01, 5, 10);
	auto source_data = make_data_12(0x07, 5, 10);

	auto merge = make_record("CELL", make_sub("NAME", make_string("cell")) + make_sub("DATA", merge_data));

	auto source = make_record("CELL", make_sub("NAME", make_string("cell")) + make_sub("DATA", source_data));

	auto result = merge_patch_ops_t::patch_field(merge, source, "CELL", "DATA", 12, 1, 0);

	REQUIRE(result.success);

	auto patched_subs = sub_record_merge_t::parse_sub_records(result.content);
	uint32_t patched_flags = 0;
	std::memcpy(&patched_flags, patched_subs[1].data.data(), 4);
	REQUIRE(patched_flags == 0x07);

	int32_t patched_grid_x = 0;
	std::memcpy(&patched_grid_x, patched_subs[1].data.data() + 4, 4);
	REQUIRE(patched_grid_x == 5);
}

TEST_CASE("merge_patch_ops_t::patch_field, copies whole sub-record if not in merge", "[u]")
{
	auto source_data = make_data_12(0x01, 7, 3);

	auto merge = make_record("CELL", make_sub("NAME", make_string("cell")));

	auto source = make_record("CELL", make_sub("NAME", make_string("cell")) + make_sub("DATA", source_data));

	auto result = merge_patch_ops_t::patch_field(merge, source, "CELL", "DATA", 12, 1, 1);

	REQUIRE(result.success);

	auto patched_subs = sub_record_merge_t::parse_sub_records(result.content);
	REQUIRE(patched_subs.size() == 2);
	REQUIRE(patched_subs[1].type == "DATA");
	REQUIRE(patched_subs[1].data == source_data);
}

TEST_CASE("merge_patch_ops_t::patch_field, patches grouped CLDT major skill field", "[u]")
{
	static constexpr size_t cldt_size = 60;
	static constexpr size_t major_three_offset = 32;
	static constexpr int major_three_field_idx = 5;

	std::string merge_data(cldt_size, '\0');
	std::string source_data(cldt_size, '\0');
	uint32_t merge_skill = 3;
	uint32_t source_skill = 11;
	std::memcpy(merge_data.data() + major_three_offset, &merge_skill, 4);
	std::memcpy(source_data.data() + major_three_offset, &source_skill, 4);

	auto merge = make_record("CLAS", make_sub("NAME", make_string("test_class")) + make_sub("CLDT", merge_data));
	auto source = make_record("CLAS", make_sub("NAME", make_string("test_class")) + make_sub("CLDT", source_data));

	auto result = merge_patch_ops_t::patch_field(merge, source, "CLAS", "CLDT", cldt_size, 1, major_three_field_idx);

	REQUIRE(result.success);

	auto patched_subs = sub_record_merge_t::parse_sub_records(result.content);
	REQUIRE(patched_subs.size() == 2);

	uint32_t patched_skill = 0;
	std::memcpy(&patched_skill, patched_subs[1].data.data() + major_three_offset, 4);
	REQUIRE(patched_skill == source_skill);
}

TEST_CASE("merge_patch_ops_t::patch_bit, sets flag bit from source", "[u]")
{
	std::string merge_data(4, '\0');
	std::string source_data(4, '\0');
	source_data[2] = static_cast<char>(0x01);

	auto merge = make_record("BODY", make_sub("NAME", make_string("id")) + make_sub("BYDT", merge_data));
	auto source = make_record("BODY", make_sub("NAME", make_string("id")) + make_sub("BYDT", source_data));

	merge_patch_ops_t::bit_patch_params_t params;
	params.record_type = "BODY";
	params.sub_type = "BYDT";
	params.sub_size = 4;
	params.binary_idx = 1;
	params.field_idx = 2;
	params.bit_index = 0;

	auto result = merge_patch_ops_t::patch_bit(merge, source, params);

	REQUIRE(result.success);

	auto patched_subs = sub_record_merge_t::parse_sub_records(result.content);
	REQUIRE((static_cast<unsigned char>(patched_subs[1].data[2]) & 0x01) != 0);
}

TEST_CASE("merge_patch_ops_t::patch_bit, clears flag bit and preserves others", "[u]")
{
	std::string merge_data(4, '\0');
	merge_data[2] = static_cast<char>(0x03);
	std::string source_data(4, '\0');
	source_data[2] = static_cast<char>(0x02);

	auto merge = make_record("BODY", make_sub("NAME", make_string("id")) + make_sub("BYDT", merge_data));
	auto source = make_record("BODY", make_sub("NAME", make_string("id")) + make_sub("BYDT", source_data));

	merge_patch_ops_t::bit_patch_params_t params;
	params.record_type = "BODY";
	params.sub_type = "BYDT";
	params.sub_size = 4;
	params.binary_idx = 1;
	params.field_idx = 2;
	params.bit_index = 0;

	auto result = merge_patch_ops_t::patch_bit(merge, source, params);

	REQUIRE(result.success);

	auto patched_subs = sub_record_merge_t::parse_sub_records(result.content);
	const auto flags = static_cast<unsigned char>(patched_subs[1].data[2]);
	REQUIRE((flags & 0x01) == 0);
	REQUIRE((flags & 0x02) != 0);
}

TEST_CASE("merge_patch_ops_t::patch_bit, copies whole sub-record if not in merge", "[u]")
{
	std::string source_data(4, '\0');
	source_data[2] = static_cast<char>(0x01);

	auto merge = make_record("BODY", make_sub("NAME", make_string("id")));
	auto source = make_record("BODY", make_sub("NAME", make_string("id")) + make_sub("BYDT", source_data));

	merge_patch_ops_t::bit_patch_params_t params;
	params.record_type = "BODY";
	params.sub_type = "BYDT";
	params.sub_size = 4;
	params.binary_idx = 1;
	params.field_idx = 2;
	params.bit_index = 0;

	auto result = merge_patch_ops_t::patch_bit(merge, source, params);

	REQUIRE(result.success);

	auto patched_subs = sub_record_merge_t::parse_sub_records(result.content);
	REQUIRE(patched_subs.size() == 2);
	REQUIRE(patched_subs[1].type == "BYDT");
}

TEST_CASE("merge_patch_ops_t::patch_bit, bit_index out of range fails", "[u]")
{
	std::string data(4, '\0');
	auto merge = make_record("BODY", make_sub("NAME", make_string("id")) + make_sub("BYDT", data));
	auto source = make_record("BODY", make_sub("NAME", make_string("id")) + make_sub("BYDT", data));

	merge_patch_ops_t::bit_patch_params_t params;
	params.record_type = "BODY";
	params.sub_type = "BYDT";
	params.sub_size = 4;
	params.binary_idx = 1;
	params.field_idx = 2;
	params.bit_index = 99;

	auto result = merge_patch_ops_t::patch_bit(merge, source, params);

	REQUIRE_FALSE(result.success);
}

TEST_CASE("merge_patch_ops_t::patch_field, no schema returns failure", "[u]")
{
	auto merge = make_record("NPC_", make_sub("NAME", make_string("id")) + make_sub("XXXX", std::string(8, '\0')));

	auto source = make_record("NPC_", make_sub("NAME", make_string("id")) + make_sub("XXXX", std::string(8, '\0')));

	auto result = merge_patch_ops_t::patch_field(merge, source, "NPC_", "XXXX", 8, 1, 0);

	REQUIRE_FALSE(result.success);
}

TEST_CASE("merge_patch_ops_t::patch_field, field_idx out of range fails", "[u]")
{
	auto data = make_data_12(0x01, 5, 10);

	auto merge = make_record("CELL", make_sub("NAME", make_string("cell")) + make_sub("DATA", data));

	auto source = make_record("CELL", make_sub("NAME", make_string("cell")) + make_sub("DATA", data));

	auto result = merge_patch_ops_t::patch_field(merge, source, "CELL", "DATA", 12, 1, 99);

	REQUIRE_FALSE(result.success);
}

TEST_CASE("merge_patch_ops_t::extract_sub_type_from_field_name, type with description", "[u]")
{
	REQUIRE(merge_patch_ops_t::extract_sub_type_from_field_name("BNAM - Male Part Name") == "BNAM");
	REQUIRE(merge_patch_ops_t::extract_sub_type_from_field_name("CNAM - Female Part Name") == "CNAM");
	REQUIRE(merge_patch_ops_t::extract_sub_type_from_field_name("INDX - Armor Index") == "INDX");
}

TEST_CASE("merge_patch_ops_t::extract_sub_type_from_field_name, type only", "[u]")
{
	REQUIRE(merge_patch_ops_t::extract_sub_type_from_field_name("BNAM") == "BNAM");
	REQUIRE(merge_patch_ops_t::extract_sub_type_from_field_name("DATA") == "DATA");
}

TEST_CASE("merge_patch_ops_t::extract_sub_type_from_field_name, too short returns empty", "[u]")
{
	REQUIRE(merge_patch_ops_t::extract_sub_type_from_field_name("AB") == "");
	REQUIRE(merge_patch_ops_t::extract_sub_type_from_field_name("") == "");
}

TEST_CASE("merge_patch_ops_t::is_content_matched_type, NPCS and NPCO", "[u]")
{
	REQUIRE(merge_patch_ops_t::is_content_matched_type("NPCS"));
	REQUIRE(merge_patch_ops_t::is_content_matched_type("NPCO"));
	REQUIRE_FALSE(merge_patch_ops_t::is_content_matched_type("FNAM"));
	REQUIRE_FALSE(merge_patch_ops_t::is_content_matched_type("DATA"));
	REQUIRE_FALSE(merge_patch_ops_t::is_content_matched_type("NAME"));
}

TEST_CASE("merge_patch_ops_t::patch_sub_record, NPCS appends without replacing", "[u]")
{
	std::string npcs_data(32, '\0');
	std::memcpy(npcs_data.data(), "blood of the north", 18);

	std::string npcs_new(32, '\0');
	std::memcpy(npcs_new.data(), "_trollkin", 9);

	auto merge = make_record("BSGN", make_sub("NAME", make_string("id")) + make_sub("NPCS", npcs_data));

	auto source = make_record("BSGN", make_sub("NAME", make_string("id")) + make_sub("NPCS", npcs_new));

	auto result = merge_patch_ops_t::patch_sub_record(merge, source, "NPCS", 1);

	REQUIRE(result.success);
	REQUIRE(result.content.find("blood of the north") != std::string::npos);
	REQUIRE(result.content.find("_trollkin") != std::string::npos);
}

TEST_CASE("merge_patch_ops_t::patch_sub_record, NPCS skips duplicate", "[u]")
{
	std::string npcs_data(32, '\0');
	std::memcpy(npcs_data.data(), "same_spell", 10);

	auto merge = make_record("BSGN", make_sub("NAME", make_string("id")) + make_sub("NPCS", npcs_data));

	auto source = make_record("BSGN", make_sub("NAME", make_string("id")) + make_sub("NPCS", npcs_data));

	auto result = merge_patch_ops_t::patch_sub_record(merge, source, "NPCS", 1);

	REQUIRE(result.success);

	size_t count = 0;
	size_t search_pos = 0;
	while ((search_pos = result.content.find("NPCS", search_pos)) != std::string::npos)
	{
		++count;
		search_pos += 4;
	}
	REQUIRE(count == 1);
}
