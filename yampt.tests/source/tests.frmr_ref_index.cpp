#include <catch2/catch_all.hpp>
#include <decoder/sub_record_iter.hpp>
#include <decoder/conflict_slots.hpp>
#include <cstring>
#include <string>

static std::string make_uint32(uint32_t value)
{
	std::string result(4, '\0');
	std::memcpy(result.data(), &value, 4);
	return result;
}

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

static std::string make_position()
{
	return std::string(24, '\0');
}

TEST_CASE("read_frmr_ref_index, extracts lower 24 bits", "[u]")
{
	auto data = make_uint32(0x01035019);
	REQUIRE(read_frmr_ref_index(data.data(), data.size()) == 0x035019);
}

TEST_CASE("read_frmr_ref_index, zero master byte passes through", "[u]")
{
	auto data = make_uint32(192882);
	REQUIRE(read_frmr_ref_index(data.data(), data.size()) == 192882);
}

TEST_CASE("read_frmr_ref_index, different master same ref match", "[u]")
{
	auto morrowind_frmr = make_uint32(0x00035019);
	auto plugin_frmr = make_uint32(0x01035019);

	const auto morrowind_idx = read_frmr_ref_index(morrowind_frmr.data(), morrowind_frmr.size());
	const auto plugin_idx = read_frmr_ref_index(plugin_frmr.data(), plugin_frmr.size());

	REQUIRE(morrowind_idx == plugin_idx);
}

TEST_CASE("read_frmr_ref_index, different refs dont match", "[u]")
{
	auto ref_a = make_uint32(0x01000001);
	auto ref_b = make_uint32(0x01000002);

	REQUIRE(read_frmr_ref_index(ref_a.data(), ref_a.size()) != read_frmr_ref_index(ref_b.data(), ref_b.size()));
}

TEST_CASE("read_frmr_ref_index, short data returns zero", "[u]")
{
	std::string short_data(2, '\0');
	REQUIRE(read_frmr_ref_index(short_data.data(), short_data.size()) == 0);
}

TEST_CASE("read_frmr_ref_index, max 24bit value", "[u]")
{
	auto data = make_uint32(0xFF'FFFFFF);
	REQUIRE(read_frmr_ref_index(data.data(), data.size()) == 0x00FFFFFF);
}

TEST_CASE("build_cell_slots, refs with same lower 24 bits aligned", "[u]")
{
	auto header_subs = make_sub("NAME", make_string("TestCell"))
	                 + make_sub("DATA", std::string(12, '\0'));

	auto ref_morrowind = make_sub("FRMR", make_uint32(0x00020164))
	                   + make_sub("NAME", make_string("door_01"))
	                   + make_sub("DATA", make_position());

	auto ref_plugin = make_sub("FRMR", make_uint32(0x01020164))
	                + make_sub("NAME", make_string("door_01"))
	                + make_sub("DATA", make_position());

	auto content_v1 = make_record("CELL", header_subs + ref_morrowind);
	auto content_v2 = make_record("CELL", header_subs + ref_plugin);

	std::vector<std::string> versions = { content_v1, content_v2 };
	std::vector<bool> deleted = { false, false };
	auto result = build_conflict_slots("CELL", versions, deleted);

	bool found_frmr_aligned = false;
	for (const auto & slot : result.aligned)
	{
		if (slot.key.type != "FRMR")
			continue;

		if (slot.indices[0] != SIZE_MAX && slot.indices[1] != SIZE_MAX)
		{
			found_frmr_aligned = true;
			break;
		}
	}

	REQUIRE(found_frmr_aligned);
}

TEST_CASE("build_cell_slots, different refs stay separate", "[u]")
{
	auto header_subs = make_sub("NAME", make_string("TestCell"))
	                 + make_sub("DATA", std::string(12, '\0'));

	auto ref_a = make_sub("FRMR", make_uint32(0x00000001))
	           + make_sub("NAME", make_string("item_a"))
	           + make_sub("DATA", make_position());

	auto ref_b = make_sub("FRMR", make_uint32(0x00000002))
	           + make_sub("NAME", make_string("item_b"))
	           + make_sub("DATA", make_position());

	auto content_v1 = make_record("CELL", header_subs + ref_a);
	auto content_v2 = make_record("CELL", header_subs + ref_b);

	std::vector<std::string> versions = { content_v1, content_v2 };
	std::vector<bool> deleted = { false, false };
	auto result = build_conflict_slots("CELL", versions, deleted);

	int frmr_slot_count = 0;
	for (const auto & slot : result.aligned)
	{
		if (slot.key.type == "FRMR")
			++frmr_slot_count;
	}

	REQUIRE(frmr_slot_count == 2);
}

TEST_CASE("compute_conflict_all_skip_empty, ignores non-existent", "[u]")
{
	std::vector<std::string> values = { "hello", non_existent_value, "hello" };
	REQUIRE(compute_conflict_all_skip_empty(values) == conflict_all_t::no_conflict);
}

TEST_CASE("compute_conflict_all_skip_empty, real empty is not skipped", "[u]")
{
	std::vector<std::string> values = { "hello", "", "hello" };
	REQUIRE(compute_conflict_all_skip_empty(values) != conflict_all_t::no_conflict);
}

TEST_CASE("compute_conflict_all_skip_empty, all non-existent", "[u]")
{
	std::vector<std::string> values = { non_existent_value, non_existent_value, non_existent_value };
	REQUIRE(compute_conflict_all_skip_empty(values) == conflict_all_t::only_one);
}

TEST_CASE("compute_conflict_all_skip_empty, single present value", "[u]")
{
	std::vector<std::string> values = { non_existent_value, "data", non_existent_value };
	REQUIRE(compute_conflict_all_skip_empty(values) == conflict_all_t::only_one);
}

TEST_CASE("compute_conflict_all_skip_empty, override detected", "[u]")
{
	std::vector<std::string> values = { "old", non_existent_value, "new" };
	REQUIRE(compute_conflict_all_skip_empty(values) == conflict_all_t::override_benign);
}

TEST_CASE("compute_conflict_all_skip_empty, conflict detected", "[u]")
{
	std::vector<std::string> values = { "aaa", "bbb", "ccc" };
	REQUIRE(compute_conflict_all_skip_empty(values) == conflict_all_t::conflict);
}

TEST_CASE("compute_conflict_this_skip_empty, non-existent gets unknown", "[u]")
{
	std::vector<std::string> values = { "master", non_existent_value, "override" };
	const auto result = compute_conflict_this_skip_empty(values);

	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::unknown);
	REQUIRE(result[2] == conflict_this_t::override_wins);
}

TEST_CASE("compute_conflict_this_skip_empty, identical across present", "[u]")
{
	std::vector<std::string> values = { "same", non_existent_value, "same" };
	const auto result = compute_conflict_this_skip_empty(values);

	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::unknown);
	REQUIRE(result[2] == conflict_this_t::identical_to_master);
}

TEST_CASE("compute_conflict_this_skip_empty, real empty is a value", "[u]")
{
	std::vector<std::string> values = { "text", "", "text" };
	const auto result = compute_conflict_this_skip_empty(values);

	REQUIRE(result[1] == conflict_this_t::conflict_loses);
}

TEST_CASE("find_conflict_policy, CELL wildcard returns skip", "[u]")
{
	const auto policy = find_conflict_policy("CELL", "DATA");
	REQUIRE(policy.skip_non_existent == true);
	REQUIRE(policy.ignore_conflict == false);
}

TEST_CASE("find_conflict_policy, CELL NAM0 returns ignore", "[u]")
{
	const auto policy = find_conflict_policy("CELL", "NAM0");
	REQUIRE(policy.skip_non_existent == false);
	REQUIRE(policy.ignore_conflict == true);
}

TEST_CASE("find_conflict_policy, unknown type returns default", "[u]")
{
	const auto policy = find_conflict_policy("NPC_", "DATA");
	REQUIRE(policy.skip_non_existent == false);
	REQUIRE(policy.ignore_conflict == false);
}

TEST_CASE("non_existent_value, cannot collide with format_value_full", "[u]")
{
	REQUIRE(non_existent_value[0] == '\0');
	REQUIRE(non_existent_value.size() == 4);
}
