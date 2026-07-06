#include <catch2/catch_all.hpp>
#include <decoder/conflict_slots.hpp>
#include <decoder/sub_record_iter.hpp>
#include <scanner/record_conflict.hpp>
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

TEST_CASE("sub_record_iter::read_frmr_ref_index, extracts lower 24 bits", "[u]")
{
	auto data = make_uint32(0x01035019);
	REQUIRE(read_frmr_ref_index(data.data(), data.size()) == 0x035019);
}

TEST_CASE("sub_record_iter::read_frmr_ref_index, zero master byte passes through", "[u]")
{
	auto data = make_uint32(192882);
	REQUIRE(read_frmr_ref_index(data.data(), data.size()) == 192882);
}

TEST_CASE("sub_record_iter::read_frmr_ref_index, different master same ref match", "[u]")
{
	auto morrowind_frmr = make_uint32(0x00035019);
	auto plugin_frmr = make_uint32(0x01035019);

	const auto morrowind_idx = read_frmr_ref_index(morrowind_frmr.data(), morrowind_frmr.size());
	const auto plugin_idx = read_frmr_ref_index(plugin_frmr.data(), plugin_frmr.size());

	REQUIRE(morrowind_idx == plugin_idx);
}

TEST_CASE("sub_record_iter::read_frmr_ref_index, different refs dont match", "[u]")
{
	auto ref_a = make_uint32(0x01000001);
	auto ref_b = make_uint32(0x01000002);

	REQUIRE(read_frmr_ref_index(ref_a.data(), ref_a.size()) != read_frmr_ref_index(ref_b.data(), ref_b.size()));
}

TEST_CASE("sub_record_iter::read_frmr_ref_index, short data returns zero", "[u]")
{
	std::string short_data(2, '\0');
	REQUIRE(read_frmr_ref_index(short_data.data(), short_data.size()) == 0);
}

TEST_CASE("sub_record_iter::read_frmr_ref_index, max 24bit value", "[u]")
{
	auto data = make_uint32(0xFF'FFFFFF);
	REQUIRE(read_frmr_ref_index(data.data(), data.size()) == 0x00FFFFFF);
}

TEST_CASE("conflict_slots::build, refs with same lower 24 bits aligned", "[u]")
{
	auto header_subs = make_sub("NAME", make_string("TestCell")) + make_sub("DATA", std::string(12, '\0'));

	auto ref_morrowind = make_sub("FRMR", make_uint32(0x00020164)) + make_sub("NAME", make_string("door_01")) +
	                     make_sub("DATA", make_position());

	auto ref_plugin = make_sub("FRMR", make_uint32(0x01020164)) + make_sub("NAME", make_string("door_01")) +
	                  make_sub("DATA", make_position());

	auto content_v1 = make_record("CELL", header_subs + ref_morrowind);
	auto content_v2 = make_record("CELL", header_subs + ref_plugin);

	std::vector<std::string> versions = { content_v1, content_v2 };
	std::vector<bool> deleted = { false, false };
	auto result = conflict_slots::build("CELL", versions, deleted);

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

TEST_CASE("conflict_slots::build, different refs stay separate", "[u]")
{
	auto header_subs = make_sub("NAME", make_string("TestCell")) + make_sub("DATA", std::string(12, '\0'));

	auto ref_a = make_sub("FRMR", make_uint32(0x00000001)) + make_sub("NAME", make_string("item_a")) +
	             make_sub("DATA", make_position());

	auto ref_b = make_sub("FRMR", make_uint32(0x00000002)) + make_sub("NAME", make_string("item_b")) +
	             make_sub("DATA", make_position());

	auto content_v1 = make_record("CELL", header_subs + ref_a);
	auto content_v2 = make_record("CELL", header_subs + ref_b);

	std::vector<std::string> versions = { content_v1, content_v2 };
	std::vector<bool> deleted = { false, false };
	auto result = conflict_slots::build("CELL", versions, deleted);

	int frmr_slot_count = 0;
	for (const auto & slot : result.aligned)
	{
		if (slot.key.type == "FRMR")
			++frmr_slot_count;
	}

	REQUIRE(frmr_slot_count == 2);
}

TEST_CASE("record_conflict::compute_conflict_all_skip_empty, ignores non-existent", "[u]")
{
	std::vector<std::string> values = { "hello", non_existent_value, "hello" };
	REQUIRE(record_conflict::compute_conflict_all_skip_empty(values) == conflict_all_t::no_conflict);
}

TEST_CASE("record_conflict::compute_conflict_all_skip_empty, real empty is not skipped", "[u]")
{
	std::vector<std::string> values = { "hello", "", "hello" };
	REQUIRE(record_conflict::compute_conflict_all_skip_empty(values) != conflict_all_t::no_conflict);
}

TEST_CASE("record_conflict::compute_conflict_all_skip_empty, all non-existent", "[u]")
{
	std::vector<std::string> values = { non_existent_value, non_existent_value, non_existent_value };
	REQUIRE(record_conflict::compute_conflict_all_skip_empty(values) == conflict_all_t::only_one);
}

TEST_CASE("record_conflict::compute_conflict_all_skip_empty, single present value", "[u]")
{
	std::vector<std::string> values = { non_existent_value, "data", non_existent_value };
	REQUIRE(record_conflict::compute_conflict_all_skip_empty(values) == conflict_all_t::only_one);
}

TEST_CASE("record_conflict::compute_conflict_all_skip_empty, override detected", "[u]")
{
	std::vector<std::string> values = { "old", non_existent_value, "new" };
	REQUIRE(record_conflict::compute_conflict_all_skip_empty(values) == conflict_all_t::override_benign);
}

TEST_CASE("record_conflict::compute_conflict_all_skip_empty, conflict detected", "[u]")
{
	std::vector<std::string> values = { "aaa", "bbb", "ccc" };
	REQUIRE(record_conflict::compute_conflict_all_skip_empty(values) == conflict_all_t::conflict);
}

TEST_CASE("record_conflict::compute_conflict_this_skip_empty, non-existent gets unknown", "[u]")
{
	std::vector<std::string> values = { "master", non_existent_value, "override" };
	const auto result = record_conflict::compute_conflict_this_skip_empty(values);

	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::unknown);
	REQUIRE(result[2] == conflict_this_t::override_wins);
}

TEST_CASE("record_conflict::compute_conflict_this_skip_empty, identical across present", "[u]")
{
	std::vector<std::string> values = { "same", non_existent_value, "same" };
	const auto result = record_conflict::compute_conflict_this_skip_empty(values);

	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::unknown);
	REQUIRE(result[2] == conflict_this_t::identical_to_master);
}

TEST_CASE("record_conflict::compute_conflict_this_skip_empty, real empty is a value", "[u]")
{
	std::vector<std::string> values = { "text", "", "text" };
	const auto result = record_conflict::compute_conflict_this_skip_empty(values);

	REQUIRE(result[1] == conflict_this_t::conflict_loses);
}

TEST_CASE("record_conflict::find_conflict_policy, CELL wildcard returns skip", "[u]")
{
	const auto policy = record_conflict::find_conflict_policy("CELL", "DATA");
	REQUIRE(policy.skip_non_existent == true);
	REQUIRE(policy.ignore_conflict == false);
}

TEST_CASE("record_conflict::find_conflict_policy, CELL NAM0 returns ignore", "[u]")
{
	const auto policy = record_conflict::find_conflict_policy("CELL", "NAM0");
	REQUIRE(policy.skip_non_existent == false);
	REQUIRE(policy.ignore_conflict == true);
}

TEST_CASE("record_conflict::find_conflict_policy, unknown type returns default", "[u]")
{
	const auto policy = record_conflict::find_conflict_policy("NPC_", "DATA");
	REQUIRE(policy.skip_non_existent == false);
	REQUIRE(policy.ignore_conflict == false);
}

TEST_CASE("record_conflict::non_existent_value, cannot collide with format_value_full", "[u]")
{
	REQUIRE(non_existent_value[0] == '\0');
	REQUIRE(non_existent_value.size() == 4);
}

TEST_CASE("record_conflict::find_conflict_policy, CELL NAME inherits wildcard", "[u]")
{
	const auto policy = record_conflict::find_conflict_policy("CELL", "NAME");
	REQUIRE(policy.skip_non_existent == true);
	REQUIRE(policy.ignore_conflict == false);
}

TEST_CASE("record_conflict::find_conflict_policy, CELL WHGT inherits wildcard", "[u]")
{
	const auto policy = record_conflict::find_conflict_policy("CELL", "WHGT");
	REQUIRE(policy.skip_non_existent == true);
	REQUIRE(policy.ignore_conflict == false);
}

TEST_CASE("record_conflict::find_conflict_policy, NAM0 overrides wildcard", "[u]")
{
	const auto policy = record_conflict::find_conflict_policy("CELL", "NAM0");
	REQUIRE(policy.skip_non_existent == false);
	REQUIRE(policy.ignore_conflict == true);
}

TEST_CASE("record_conflict::find_conflict_policy, NPC_ FNAM returns default", "[u]")
{
	const auto policy = record_conflict::find_conflict_policy("NPC_", "FNAM");
	REQUIRE(policy.skip_non_existent == false);
	REQUIRE(policy.ignore_conflict == false);
}

TEST_CASE("record_conflict::find_conflict_policy, DIAL NAME returns default", "[u]")
{
	const auto policy = record_conflict::find_conflict_policy("DIAL", "NAME");
	REQUIRE(policy.skip_non_existent == false);
	REQUIRE(policy.ignore_conflict == false);
}

TEST_CASE("record_conflict::compute_conflict_all_skip_empty, two present same with gap", "[u]")
{
	std::vector<std::string> values = { "X", non_existent_value, non_existent_value, "X" };
	REQUIRE(record_conflict::compute_conflict_all_skip_empty(values) == conflict_all_t::no_conflict);
}

TEST_CASE("record_conflict::compute_conflict_all_skip_empty, two present differ with gap", "[u]")
{
	std::vector<std::string> values = { "X", non_existent_value, "Y" };
	REQUIRE(record_conflict::compute_conflict_all_skip_empty(values) == conflict_all_t::override_benign);
}

TEST_CASE("record_conflict::compute_conflict_this_skip_empty, all non-existent yields unknown", "[u]")
{
	std::vector<std::string> values = { non_existent_value, non_existent_value };
	const auto result = record_conflict::compute_conflict_this_skip_empty(values);
	REQUIRE(result[0] == conflict_this_t::unknown);
	REQUIRE(result[1] == conflict_this_t::unknown);
}

TEST_CASE("record_conflict::compute_conflict_this_skip_empty, override with non-existent middle", "[u]")
{
	std::vector<std::string> values = { "A", non_existent_value, "B" };
	const auto result = record_conflict::compute_conflict_this_skip_empty(values);
	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::unknown);
	REQUIRE(result[2] == conflict_this_t::override_wins);
}

TEST_CASE("record_conflict::compute_conflict_this_skip_empty, conflict with three present", "[u]")
{
	std::vector<std::string> values = { "A", "B", non_existent_value, "C" };
	const auto result = record_conflict::compute_conflict_this_skip_empty(values);
	REQUIRE(result[0] == conflict_this_t::master);
	REQUIRE(result[1] == conflict_this_t::conflict_loses);
	REQUIRE(result[2] == conflict_this_t::unknown);
	REQUIRE(result[3] == conflict_this_t::conflict_wins);
}

TEST_CASE("conflict_slots::build, NAM0 appears in header slots", "[u]")
{
	auto header_subs = make_sub("NAME", make_string("TestCell")) + make_sub("DATA", std::string(12, '\0')) +
	                   make_sub("NAM0", make_uint32(5));

	auto ref = make_sub("FRMR", make_uint32(1)) + make_sub("NAME", make_string("object_01")) +
	           make_sub("DATA", make_position());

	auto content = make_record("CELL", header_subs + ref);

	std::vector<std::string> versions = { content };
	std::vector<bool> deleted = { false };
	auto result = conflict_slots::build("CELL", versions, deleted);

	bool found_nam0 = false;
	for (const auto & slot : result.aligned)
	{
		if (slot.key.type == "NAM0")
		{
			found_nam0 = true;
			break;
		}
	}

	REQUIRE(found_nam0);
}

TEST_CASE("conflict_slots::build, header sub-records before first FRMR", "[u]")
{
	auto header_subs = make_sub("NAME", make_string("Interior")) + make_sub("DATA", std::string(12, '\0')) +
	                   make_sub("WHGT", std::string(4, '\0')) + make_sub("AMBI", std::string(16, '\0'));

	auto ref =
	    make_sub("FRMR", make_uint32(100)) + make_sub("NAME", make_string("chest")) + make_sub("DATA", make_position());

	auto content = make_record("CELL", header_subs + ref);

	std::vector<std::string> versions = { content };
	std::vector<bool> deleted = { false };
	auto result = conflict_slots::build("CELL", versions, deleted);

	int header_types_found = 0;
	for (const auto & slot : result.aligned)
	{
		if (slot.key.type == "FRMR")
			break;

		if (slot.key.type == "NAME" || slot.key.type == "DATA" || slot.key.type == "WHGT" || slot.key.type == "AMBI")
			++header_types_found;
	}

	REQUIRE(header_types_found == 4);
}

TEST_CASE("record_conflict::compute_conflict_all, non_existent_value differs from empty", "[u]")
{
	std::vector<std::string> values = { "", non_existent_value };
	REQUIRE(record_conflict::compute_conflict_all(values) == conflict_all_t::override_benign);
}

TEST_CASE("record_conflict::compute_conflict_all_skip_empty, empty string causes conflict", "[u]")
{
	std::vector<std::string> values = { "value", "", "value" };
	REQUIRE(record_conflict::compute_conflict_all_skip_empty(values) == conflict_all_t::conflict);
}

TEST_CASE("record_conflict::find_conflict_policy, CELL AMBI inherits wildcard", "[u]")
{
	const auto policy = record_conflict::find_conflict_policy("CELL", "AMBI");
	REQUIRE(policy.skip_non_existent == true);
	REQUIRE(policy.ignore_conflict == false);
}

TEST_CASE("record_conflict::find_conflict_policy, CELL FRMR inherits wildcard", "[u]")
{
	const auto policy = record_conflict::find_conflict_policy("CELL", "FRMR");
	REQUIRE(policy.skip_non_existent == true);
	REQUIRE(policy.ignore_conflict == false);
}

TEST_CASE("record_conflict::find_conflict_policy, LEVI INAM returns default", "[u]")
{
	const auto policy = record_conflict::find_conflict_policy("LEVI", "INAM");
	REQUIRE(policy.skip_non_existent == false);
	REQUIRE(policy.ignore_conflict == false);
}
