#include <catch2/catch_all.hpp>
#include <scanner/sub_record_merge.hpp>
#include <utility/tools.hpp>
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

static std::string make_levi_header(const std::string & list_id, uint32_t list_flags, uint8_t chance_none, uint32_t item_count)
{
	return make_sub("NAME", make_string(list_id))
	     + make_sub("DATA", make_uint32(list_flags))
	     + make_sub("NNAM", std::string(1, static_cast<char>(chance_none)))
	     + make_sub("INDX", make_uint32(item_count));
}

// ============================================================================
// Requirement 2: Occurrence-Counted Leveled List Merge
// ============================================================================

TEST_CASE("leveled_list_merge, max occurrence preserved", "[u]")
{
	auto header = make_levi_header("list_id", 1, 0, 1);
	auto first_subs = header + make_levi_entry("iron_sword", 1);
	auto mod_subs = make_levi_header("list_id", 1, 0, 3)
	              + make_levi_entry("iron_sword", 1)
	              + make_levi_entry("iron_sword", 1)
	              + make_levi_entry("iron_sword", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
	};

	auto result = merge_leveled_list(input);

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

TEST_CASE("leveled_list_merge, two mods add different items", "[u]")
{
	auto header = make_levi_header("list_id", 1, 0, 1);
	auto first_subs = header + make_levi_entry("item_a", 1);
	auto mod1_subs = make_levi_header("list_id", 1, 0, 2)
	               + make_levi_entry("item_a", 1)
	               + make_levi_entry("item_b", 1);
	auto mod2_subs = make_levi_header("list_id", 1, 0, 2)
	               + make_levi_entry("item_a", 1)
	               + make_levi_entry("item_c", 5);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod1_subs),
		make_record("LEVI", mod2_subs),
	};

	auto result = merge_leveled_list(input);

	REQUIRE(result.changed);
	REQUIRE(result.content.find("item_a") != std::string::npos);
	REQUIRE(result.content.find("item_b") != std::string::npos);
	REQUIRE(result.content.find("item_c") != std::string::npos);
}

TEST_CASE("leveled_list_merge, sorted by level then ident", "[u]")
{
	auto header = make_levi_header("list_id", 1, 0, 1);
	auto first_subs = header + make_levi_entry("zzz_item", 1);
	auto mod_subs = make_levi_header("list_id", 1, 0, 3)
	              + make_levi_entry("zzz_item", 1)
	              + make_levi_entry("aaa_item", 5)
	              + make_levi_entry("bbb_item", 3);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
	};

	auto result = merge_leveled_list(input);

	REQUIRE(result.changed);
	size_t pos_zzz = result.content.find("zzz_item");
	size_t pos_bbb = result.content.find("bbb_item");
	size_t pos_aaa = result.content.find("aaa_item");
	REQUIRE(pos_zzz < pos_bbb);
	REQUIRE(pos_bbb < pos_aaa);
}

TEST_CASE("leveled_list_merge, winner header used", "[u]")
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

	auto result = merge_leveled_list(input);

	size_t nnam_pos = result.content.find("NNAM");
	REQUIRE(nnam_pos != std::string::npos);
	uint8_t chance = static_cast<uint8_t>(result.content[nnam_pos + 8]);
	REQUIRE(chance == 50);
}

// ============================================================================
// Requirement 15: Leveled List Deletion Detection
// ============================================================================

TEST_CASE("leveled_list_merge, mod removes item entirely", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 2)
	                + make_levi_entry("keep_item", 1)
	                + make_levi_entry("remove_me", 1);
	auto mod_subs = make_levi_header("list_id", 1, 0, 1)
	              + make_levi_entry("keep_item", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
	};

	auto result = merge_leveled_list(input);

	REQUIRE(result.content.find("keep_item") != std::string::npos);
	REQUIRE(result.content.find("remove_me") == std::string::npos);
}

TEST_CASE("leveled_list_merge, mod reduces count partial", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 3)
	                + make_levi_entry("item_a", 1)
	                + make_levi_entry("item_a", 1)
	                + make_levi_entry("item_a", 1);
	auto mod_subs = make_levi_header("list_id", 1, 0, 1)
	              + make_levi_entry("item_a", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_subs),
	};

	auto result = merge_leveled_list(input);

	size_t count = 0;
	size_t search_pos = 0;
	while ((search_pos = result.content.find("item_a", search_pos)) != std::string::npos)
	{
		++count;
		search_pos += 6;
	}
	REQUIRE(count == 1);
}

TEST_CASE("leveled_list_merge, one mod removes one mod keeps deletion wins", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 2)
	                + make_levi_entry("contested", 1)
	                + make_levi_entry("safe_item", 1);
	auto mod_remove = make_levi_header("list_id", 1, 0, 1)
	                + make_levi_entry("safe_item", 1);
	auto mod_keep = make_levi_header("list_id", 1, 0, 3)
	              + make_levi_entry("contested", 1)
	              + make_levi_entry("contested", 1)
	              + make_levi_entry("safe_item", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod_remove),
		make_record("LEVI", mod_keep),
	};

	auto result = merge_leveled_list(input);

	REQUIRE(result.content.find("contested") == std::string::npos);
	REQUIRE(result.content.find("safe_item") != std::string::npos);
}

TEST_CASE("leveled_list_merge, no deletion when all mods keep item", "[u]")
{
	auto first_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("item_a", 1);
	auto mod1_subs = make_levi_header("list_id", 1, 0, 2)
	               + make_levi_entry("item_a", 1)
	               + make_levi_entry("item_a", 1);
	auto mod2_subs = make_levi_header("list_id", 1, 0, 1) + make_levi_entry("item_a", 1);

	leveled_list_input_t input;
	input.rec_type = "LEVI";
	input.record_id = "list_id";
	input.version_contents = {
		make_record("LEVI", first_subs),
		make_record("LEVI", mod1_subs),
		make_record("LEVI", mod2_subs),
	};

	auto result = merge_leveled_list(input);

	REQUIRE(result.content.find("item_a") != std::string::npos);
}

TEST_CASE("leveled_list_merge, LEVC creature list works same", "[u]")
{
	auto first_subs = make_levi_header("crea_list", 1, 0, 1) + make_levc_entry("rat", 1);
	auto mod_subs = make_levi_header("crea_list", 1, 0, 2)
	              + make_levc_entry("rat", 1)
	              + make_levc_entry("mudcrab", 3);

	leveled_list_input_t input;
	input.rec_type = "LEVC";
	input.record_id = "crea_list";
	input.version_contents = {
		make_record("LEVC", first_subs),
		make_record("LEVC", mod_subs),
	};

	auto result = merge_leveled_list(input);

	REQUIRE(result.changed);
	REQUIRE(result.content.find("rat") != std::string::npos);
	REQUIRE(result.content.find("mudcrab") != std::string::npos);
}
