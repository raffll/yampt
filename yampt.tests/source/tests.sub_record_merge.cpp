#include <catch2/catch_all.hpp>
#include <scanner/sub_record_merge.hpp>
#include <utility/tools.hpp>
#include <cstring>
#include <string>
#include <unordered_map>

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

static std::string make_uint32(uint32_t value)
{
	std::string result(4, '\0');
	std::memcpy(result.data(), &value, 4);
	return result;
}

static std::string make_uint16(uint16_t value)
{
	std::string result(2, '\0');
	std::memcpy(result.data(), &value, 2);
	return result;
}

static std::string make_float(float value)
{
	std::string result(4, '\0');
	std::memcpy(result.data(), &value, 4);
	return result;
}

static std::string make_bytes(const std::vector<uint8_t> & bytes)
{
	return std::string(bytes.begin(), bytes.end());
}

// ============================================================================
// Requirement 1: Three-Way Sub-Record Merge — Generic
// ============================================================================

TEST_CASE("sub_record_merge_t::merge, 2 versions returns unchanged", "[u]")
{
	auto first = make_record("NPC_", make_sub("NAME", make_string("npc_id")) + make_sub("FNAM", make_string("Name")));
	auto winner = make_record("NPC_", make_sub("NAME", make_string("npc_id")) + make_sub("FNAM", make_string("Changed")));

	merge_input_t input;
	input.rec_type = "NPC_";
	input.record_id = "npc_id";
	input.version_contents = { first, winner };

	auto result = sub_record_merge_t::merge(input);

	REQUIRE_FALSE(result.changed);
	REQUIRE(result.content == winner);
}

TEST_CASE("sub_record_merge_t::merge, intermediate modifies winner unchanged", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("Original")) + make_sub("RNAM", make_string("Imperial"));
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("NewName")) + make_sub("RNAM", make_string("Imperial"));
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("Original")) + make_sub("RNAM", make_string("Breton"));

	auto first = make_record("NPC_", subs_first);
	auto inter = make_record("NPC_", subs_inter);
	auto winner = make_record("NPC_", subs_winner);

	merge_input_t input;
	input.rec_type = "NPC_";
	input.record_id = "id";
	input.version_contents = { first, inter, winner };

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	auto expected = make_record("NPC_", make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("NewName")) + make_sub("RNAM", make_string("Breton")));
	REQUIRE(result.content == expected);
}

TEST_CASE("sub_record_merge_t::merge, both modify same sub-record winner wins", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("Original"));
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("InterName"));
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("WinnerName"));

	auto first = make_record("NPC_", subs_first);
	auto inter = make_record("NPC_", subs_inter);
	auto winner = make_record("NPC_", subs_winner);

	merge_input_t input;
	input.rec_type = "NPC_";
	input.record_id = "id";
	input.version_contents = { first, inter, winner };

	auto result = sub_record_merge_t::merge(input);

	REQUIRE_FALSE(result.changed);
	REQUIRE(result.content == winner);
}

TEST_CASE("sub_record_merge_t::merge, output identical to winner not changed", "[u]")
{
	auto subs = make_sub("NAME", make_string("id")) + make_sub("DATA", make_bytes({ 1, 2, 3, 4 }));
	auto first = make_record("ARMO", subs);
	auto inter = make_record("ARMO", subs);
	auto winner = make_record("ARMO", subs);

	merge_input_t input;
	input.rec_type = "ARMO";
	input.record_id = "id";
	input.version_contents = { first, inter, winner };

	auto result = sub_record_merge_t::merge(input);

	REQUIRE_FALSE(result.changed);
}

TEST_CASE("sub_record_merge_t::merge, 4 versions later intermediate wins", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("A")) + make_sub("DATA", make_bytes({ 10 }));
	auto subs_inter1 = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("B")) + make_sub("DATA", make_bytes({ 10 }));
	auto subs_inter2 = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("C")) + make_sub("DATA", make_bytes({ 10 }));
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("A")) + make_sub("DATA", make_bytes({ 20 }));

	merge_input_t input;
	input.rec_type = "WEAP";
	input.record_id = "id";
	input.version_contents = {
		make_record("WEAP", subs_first),
		make_record("WEAP", subs_inter1),
		make_record("WEAP", subs_inter2),
		make_record("WEAP", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	auto expected = make_record("WEAP", make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("C")) + make_sub("DATA", make_bytes({ 20 })));
	REQUIRE(result.content == expected);
}

TEST_CASE("sub_record_merge_t::merge, preserves winner header flags", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("A"));
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("B"));
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("A"));

	auto first = make_record("NPC_", subs_first, 0);
	auto inter = make_record("NPC_", subs_inter, 0);
	auto winner = make_record("NPC_", subs_winner, 0x0400);

	merge_input_t input;
	input.rec_type = "NPC_";
	input.record_id = "id";
	input.version_contents = { first, inter, winner };

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	uint32_t output_flags = 0;
	std::memcpy(&output_flags, result.content.data() + 12, 4);
	REQUIRE(output_flags == 0x0400);
}

// ============================================================================
// Sub-Record Additions by Intermediate
// ============================================================================

TEST_CASE("sub_record_merge_t::merge, added sub-record preserved", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("MODL", make_string("model.nif"));
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("MODL", make_string("model.nif")) + make_sub("CNAM", make_string("female_part"));
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("MODL", make_string("model.nif"));

	merge_input_t input;
	input.rec_type = "ARMO";
	input.record_id = "id";
	input.version_contents = {
		make_record("ARMO", subs_first),
		make_record("ARMO", subs_inter),
		make_record("ARMO", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(result.content.find("CNAM") != std::string::npos);
	REQUIRE(result.content.find("female_part") != std::string::npos);
}

TEST_CASE("sub_record_merge_t::merge, added sub-record winner has it", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("id"));
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("CNAM", make_string("inter_value"));
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("CNAM", make_string("winner_value"));

	merge_input_t input;
	input.rec_type = "ARMO";
	input.record_id = "id";
	input.version_contents = {
		make_record("ARMO", subs_first),
		make_record("ARMO", subs_inter),
		make_record("ARMO", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE_FALSE(result.changed);
	REQUIRE(result.content.find("winner_value") != std::string::npos);
}

TEST_CASE("sub_record_merge_t::merge, added sub-record with existing data", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("Name")) + make_sub("MODL", make_string("m.nif"));
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("Name")) + make_sub("MODL", make_string("m.nif")) + make_sub("BNAM", make_string("added_part"));
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("FNAM", make_string("Name")) + make_sub("MODL", make_string("m.nif"));

	merge_input_t input;
	input.rec_type = "ARMO";
	input.record_id = "id";
	input.version_contents = {
		make_record("ARMO", subs_first),
		make_record("ARMO", subs_inter),
		make_record("ARMO", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(result.content.find("BNAM") != std::string::npos);
	REQUIRE(result.content.find("added_part") != std::string::npos);
	REQUIRE(result.content.find("Name") != std::string::npos);
	REQUIRE(result.content.find("m.nif") != std::string::npos);
}

TEST_CASE("sub_record_merge_t::merge, two intermediates add different subs", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("id"));
	auto subs_inter1 = make_sub("NAME", make_string("id")) + make_sub("BNAM", make_string("part_a"));
	auto subs_inter2 = make_sub("NAME", make_string("id")) + make_sub("CNAM", make_string("part_b"));
	auto subs_winner = make_sub("NAME", make_string("id"));

	merge_input_t input;
	input.rec_type = "ARMO";
	input.record_id = "id";
	input.version_contents = {
		make_record("ARMO", subs_first),
		make_record("ARMO", subs_inter1),
		make_record("ARMO", subs_inter2),
		make_record("ARMO", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(result.content.find("BNAM") != std::string::npos);
	REQUIRE(result.content.find("part_a") != std::string::npos);
	REQUIRE(result.content.find("CNAM") != std::string::npos);
	REQUIRE(result.content.find("part_b") != std::string::npos);
}

TEST_CASE("sub_record_merge_t::merge, steel_cuirass scenario tribunal adds CNAM", "[u]")
{
	auto subs_morrowind = make_sub("NAME", make_string("steel_cuirass"))
	                    + make_sub("MODL", make_string("a\\A_Steel_Cuirass_GND.nif"))
	                    + make_sub("FNAM", make_string("Steel Cuirass"))
	                    + make_sub("INDX", make_string("Cuirass"))
	                    + make_sub("BNAM", make_string("a_steel_cuirass"));

	auto subs_tribunal = make_sub("NAME", make_string("steel_cuirass"))
	                   + make_sub("MODL", make_string("a\\A_Steel_Cuirass_GND.nif"))
	                   + make_sub("FNAM", make_string("Steel Cuirass"))
	                   + make_sub("INDX", make_string("Cuirass"))
	                   + make_sub("BNAM", make_string("a_steel_cuirass"))
	                   + make_sub("CNAM", make_string("A_Steel_Cuir_Female"));

	auto subs_bloodmoon = subs_morrowind;

	merge_input_t input;
	input.rec_type = "ARMO";
	input.record_id = "steel_cuirass";
	input.version_contents = {
		make_record("ARMO", subs_morrowind),
		make_record("ARMO", subs_tribunal),
		make_record("ARMO", subs_bloodmoon),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(result.content.find("CNAM") != std::string::npos);
	REQUIRE(result.content.find("A_Steel_Cuir_Female") != std::string::npos);
}

// ============================================================================
// Requirements 3, 4: Element-Wise Byte-Level Merge (NPC_ NPDT, CREA NPDT, AI_W)
// ============================================================================

TEST_CASE("sub_record_merge_t::merge, NPC NPDT element-wise byte merge", "[u]")
{
	std::string npdt_first(52, '\0');
	npdt_first[0] = 10;
	npdt_first[10] = 50;

	std::string npdt_inter(52, '\0');
	npdt_inter[0] = 10;
	npdt_inter[10] = 80;

	std::string npdt_winner(52, '\0');
	npdt_winner[0] = 20;
	npdt_winner[10] = 50;

	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_first);
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_inter);
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_winner);

	merge_input_t input;
	input.rec_type = "NPC_";
	input.record_id = "id";
	input.version_contents = {
		make_record("NPC_", subs_first),
		make_record("NPC_", subs_inter),
		make_record("NPC_", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	std::string expected_npdt(52, '\0');
	expected_npdt[0] = 20;
	expected_npdt[10] = 80;
	auto expected = make_record("NPC_", make_sub("NAME", make_string("id")) + make_sub("NPDT", expected_npdt));
	REQUIRE(result.content == expected);
}

TEST_CASE("sub_record_merge_t::merge, NPC NPDT winner byte wins over inter", "[u]")
{
	std::string npdt_first(52, '\0');
	npdt_first[5] = 100;

	std::string npdt_inter(52, '\0');
	npdt_inter[5] = 120;

	std::string npdt_winner(52, '\0');
	npdt_winner[5] = 130;

	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_first);
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_inter);
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_winner);

	merge_input_t input;
	input.rec_type = "NPC_";
	input.record_id = "id";
	input.version_contents = {
		make_record("NPC_", subs_first),
		make_record("NPC_", subs_inter),
		make_record("NPC_", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE_FALSE(result.changed);
}

TEST_CASE("sub_record_merge_t::merge, CREA NPDT element-wise merge", "[u]")
{
	std::string npdt_first(96, '\0');
	npdt_first[4] = 5;
	npdt_first[20] = 100;

	std::string npdt_inter(96, '\0');
	npdt_inter[4] = 5;
	npdt_inter[20] = 150;

	std::string npdt_winner(96, '\0');
	npdt_winner[4] = 10;
	npdt_winner[20] = 100;

	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_first);
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_inter);
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_winner);

	merge_input_t input;
	input.rec_type = "CREA";
	input.record_id = "id";
	input.version_contents = {
		make_record("CREA", subs_first),
		make_record("CREA", subs_inter),
		make_record("CREA", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	std::string expected_npdt(96, '\0');
	expected_npdt[4] = 10;
	expected_npdt[20] = 150;
	auto expected = make_record("CREA", make_sub("NAME", make_string("id")) + make_sub("NPDT", expected_npdt));
	REQUIRE(result.content == expected);
}

TEST_CASE("sub_record_merge_t::merge, CREA AI_W element-wise merge", "[u]")
{
	std::string aiw_first(14, '\0');
	aiw_first[0] = 100;
	aiw_first[4] = 30;

	std::string aiw_inter(14, '\0');
	aiw_inter[0] = 100;
	aiw_inter[4] = 60;

	std::string aiw_winner(14, '\0');
	aiw_winner[0] = 200;
	aiw_winner[4] = 30;

	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("AI_W", aiw_first);
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("AI_W", aiw_inter);
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("AI_W", aiw_winner);

	merge_input_t input;
	input.rec_type = "CREA";
	input.record_id = "id";
	input.version_contents = {
		make_record("CREA", subs_first),
		make_record("CREA", subs_inter),
		make_record("CREA", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	std::string expected_aiw(14, '\0');
	expected_aiw[0] = 200;
	expected_aiw[4] = 60;
	auto expected = make_record("CREA", make_sub("NAME", make_string("id")) + make_sub("AI_W", expected_aiw));
	REQUIRE(result.content == expected);
}

// ============================================================================
// Requirement 5: ENAM Per-Slot Merge
// ============================================================================

static std::string make_enam(uint16_t effect_id, uint8_t skill, uint8_t attrib, int32_t duration, int32_t min_mag, int32_t max_mag)
{
	std::string result(24, '\0');
	std::memcpy(result.data(), &effect_id, 2);
	result[2] = static_cast<char>(skill);
	result[3] = static_cast<char>(attrib);
	int32_t range = 0;
	int32_t area = 0;
	std::memcpy(result.data() + 4, &range, 4);
	std::memcpy(result.data() + 8, &duration, 4);
	std::memcpy(result.data() + 12, &min_mag, 4);
	std::memcpy(result.data() + 16, &max_mag, 4);
	std::memcpy(result.data() + 20, &area, 4);
	return result;
}

TEST_CASE("sub_record_merge_t::merge, ENAM intermediate modifies slot winner unchanged", "[u]")
{
	auto enam1 = make_enam(0, 0, 0, 10, 1, 5);
	auto enam2 = make_enam(1, 0, 0, 20, 5, 10);
	auto enam2_mod = make_enam(1, 0, 0, 60, 5, 10);

	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("ENAM", enam1) + make_sub("ENAM", enam2);
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("ENAM", enam1) + make_sub("ENAM", enam2_mod);
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("ENAM", enam1) + make_sub("ENAM", enam2);

	merge_input_t input;
	input.rec_type = "SPEL";
	input.record_id = "id";
	input.version_contents = {
		make_record("SPEL", subs_first),
		make_record("SPEL", subs_inter),
		make_record("SPEL", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	auto expected = make_record("SPEL", make_sub("NAME", make_string("id")) + make_sub("ENAM", enam1) + make_sub("ENAM", enam2_mod));
	REQUIRE(result.content == expected);
}

TEST_CASE("sub_record_merge_t::merge, ENAM intermediate adds slot", "[u]")
{
	auto enam1 = make_enam(0, 0, 0, 10, 1, 5);
	auto enam_new = make_enam(5, 0, 0, 30, 10, 20);

	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("ENAM", enam1);
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("ENAM", enam1) + make_sub("ENAM", enam_new);
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("ENAM", enam1);

	merge_input_t input;
	input.rec_type = "ENCH";
	input.record_id = "id";
	input.version_contents = {
		make_record("ENCH", subs_first),
		make_record("ENCH", subs_inter),
		make_record("ENCH", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	auto expected = make_record("ENCH", make_sub("NAME", make_string("id")) + make_sub("ENAM", enam1) + make_sub("ENAM", enam_new));
	REQUIRE(result.content == expected);
}

TEST_CASE("sub_record_merge_t::merge, ENAM winner removes slot removal stands", "[u]")
{
	auto enam1 = make_enam(0, 0, 0, 10, 1, 5);
	auto enam2 = make_enam(1, 0, 0, 20, 5, 10);
	auto enam2_mod = make_enam(1, 0, 0, 60, 5, 10);

	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("ENAM", enam1) + make_sub("ENAM", enam2);
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("ENAM", enam1) + make_sub("ENAM", enam2_mod);
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("ENAM", enam1);

	merge_input_t input;
	input.rec_type = "ALCH";
	input.record_id = "id";
	input.version_contents = {
		make_record("ALCH", subs_first),
		make_record("ALCH", subs_inter),
		make_record("ALCH", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE_FALSE(result.changed);
	REQUIRE(result.content == make_record("ALCH", make_sub("NAME", make_string("id")) + make_sub("ENAM", enam1)));
}

// ============================================================================
// Duplicate addition prevention
// ============================================================================

TEST_CASE("sub_record_merge_t::merge, duplicate addition not appended twice", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("MODL", make_string("m.nif"));
	auto subs_inter1 = make_sub("NAME", make_string("id")) + make_sub("MODL", make_string("m.nif")) + make_sub("CNAM", make_string("part"));
	auto subs_inter2 = make_sub("NAME", make_string("id")) + make_sub("MODL", make_string("m.nif")) + make_sub("CNAM", make_string("part"));
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("MODL", make_string("m.nif"));

	merge_input_t input;
	input.rec_type = "ARMO";
	input.record_id = "id";
	input.version_contents = {
		make_record("ARMO", subs_first),
		make_record("ARMO", subs_inter1),
		make_record("ARMO", subs_inter2),
		make_record("ARMO", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	size_t cnam_count = 0;
	size_t pos = 0;
	while ((pos = result.content.find("CNAM", pos)) != std::string::npos)
	{
		++cnam_count;
		pos += 4;
	}
	REQUIRE(cnam_count == 1);
}

// ============================================================================
// Autocalc NPC intermediate skipped
// ============================================================================

TEST_CASE("sub_record_merge_t::merge, autocalc intermediate skipped", "[u]")
{
	std::string npdt_52(52, '\0');
	npdt_52[0] = 23;
	npdt_52[4] = 100;

	std::string npdt_12(12, '\0');
	npdt_12[0] = 23;
	npdt_12[4] = 50;

	std::string npdt_52_winner(52, '\0');
	npdt_52_winner[0] = 23;
	npdt_52_winner[4] = 100;
	npdt_52_winner[8] = 75;

	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_52);
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_12);
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_52_winner);

	merge_input_t input;
	input.rec_type = "NPC_";
	input.record_id = "id";
	input.version_contents = {
		make_record("NPC_", subs_first),
		make_record("NPC_", subs_inter),
		make_record("NPC_", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE_FALSE(result.changed);
	REQUIRE(result.content == make_record("NPC_", subs_winner));
}

// ============================================================================
// Element-wise merge requires same size
// ============================================================================

TEST_CASE("sub_record_merge_t::merge, element-wise skipped on size mismatch", "[u]")
{
	std::string npdt_52(52, '\0');
	npdt_52[4] = 100;

	std::string npdt_12(12, '\0');
	npdt_12[4] = 50;

	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_52);
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_12);
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_52);

	merge_input_t input;
	input.rec_type = "NPC_";
	input.record_id = "id";
	input.version_contents = {
		make_record("NPC_", subs_first),
		make_record("NPC_", subs_inter),
		make_record("NPC_", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE_FALSE(result.changed);
}

// ============================================================================
// ENAM byte-level merge (not whole-slot)
// ============================================================================

TEST_CASE("sub_record_merge_t::merge, ENAM per-byte merge", "[u]")
{
	auto enam_first = make_enam(79, 0, 0, 10, 2, 40);
	auto enam_inter = make_enam(79, 0, 0, 30, 2, 40);
	auto enam_winner = make_enam(79, 0, 0, 10, 1, 20);

	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("ENAM", enam_first);
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("ENAM", enam_inter);
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("ENAM", enam_winner);

	merge_input_t input;
	input.rec_type = "SPEL";
	input.record_id = "id";
	input.version_contents = {
		make_record("SPEL", subs_first),
		make_record("SPEL", subs_inter),
		make_record("SPEL", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	auto expected_enam = make_enam(79, 0, 0, 30, 1, 20);
	auto expected = make_record("SPEL", make_sub("NAME", make_string("id")) + make_sub("ENAM", expected_enam));
	REQUIRE(result.content == expected);
}

// ============================================================================
// NPCO winner only
// ============================================================================

TEST_CASE("sub_record_merge_t::merge, NPCO adds intermediate items", "[u]")
{
	std::string npco_a(36, '\0');
	npco_a[0] = 1;
	std::memcpy(&npco_a[4], "item_a", 6);

	std::string npco_b(36, '\0');
	npco_b[0] = 1;
	std::memcpy(&npco_b[4], "item_b", 6);

	std::string npco_c(36, '\0');
	npco_c[0] = 1;
	std::memcpy(&npco_c[4], "item_c", 6);

	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("NPCO", npco_a);
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("NPCO", npco_b) + make_sub("NPCO", npco_c);
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("NPCO", npco_a);

	merge_input_t input;
	input.rec_type = "NPC_";
	input.record_id = "id";
	input.version_contents = {
		make_record("NPC_", subs_first),
		make_record("NPC_", subs_inter),
		make_record("NPC_", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(result.content.find("item_a") != std::string::npos);
	REQUIRE(result.content.find("item_b") != std::string::npos);
	REQUIRE(result.content.find("item_c") != std::string::npos);
}

// ============================================================================
// CELL records skipped from 3-way merge
// ============================================================================

TEST_CASE("sub_record_merge_t::merge, CELL returns winner unchanged", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("cell")) + make_sub("DATA", make_bytes({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));
	auto subs_inter = make_sub("NAME", make_string("cell")) + make_sub("DATA", make_bytes({1, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0}));
	auto subs_winner = make_sub("NAME", make_string("cell")) + make_sub("DATA", make_bytes({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));

	auto first = make_record("CELL", subs_first);
	auto inter = make_record("CELL", subs_inter);
	auto winner = make_record("CELL", subs_winner);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "cell";
	input.version_contents = { first, inter, winner };

	auto result = sub_record_merge_t::merge(input);

	REQUIRE_FALSE(result.changed);
	REQUIRE(result.content == winner);
}

// ============================================================================
// Sub-record patching (drag and drop operation)
// ============================================================================

TEST_CASE("sub_record_merge_t, patch single sub-record by index", "[u]")
{
	auto merge_content = make_record("WEAP",
		make_sub("NAME", make_string("weapon_id")) +
		make_sub("FNAM", make_string("Old Name")) +
		make_sub("ENAM", make_string("old_enchant")));

	auto source_content = make_record("WEAP",
		make_sub("NAME", make_string("weapon_id")) +
		make_sub("FNAM", make_string("New Name")) +
		make_sub("ENAM", make_string("new_enchant")));

	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
	const auto source_subs = sub_record_merge_t::parse_sub_records(source_content);

	const size_t binary_idx = 2;
	const auto & source_sub = source_subs[binary_idx];

	const auto merge_idx = sub_record_merge_t::find_by_type_and_occurrence(merge_subs, source_sub.type, 0);
	REQUIRE(merge_idx >= 0);

	merge_subs[merge_idx].data = source_sub.data;
	const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);

	REQUIRE(patched.find("new_enchant") != std::string::npos);
	REQUIRE(patched.find("Old Name") != std::string::npos);
	REQUIRE(patched.find("old_enchant") == std::string::npos);
}

TEST_CASE("sub_record_merge_t, patch appends if not in merge", "[u]")
{
	auto merge_content = make_record("BSGN",
		make_sub("NAME", make_string("sign_id")) +
		make_sub("FNAM", make_string("Sign Name")));

	auto source_content = make_record("BSGN",
		make_sub("NAME", make_string("sign_id")) +
		make_sub("FNAM", make_string("Sign Name")) +
		make_sub("NPCS", make_string("new_ability")));

	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
	const auto source_subs = sub_record_merge_t::parse_sub_records(source_content);

	const size_t binary_idx = 2;
	const auto & source_sub = source_subs[binary_idx];

	const auto merge_idx = sub_record_merge_t::find_by_type_and_occurrence(merge_subs, source_sub.type, 0);
	REQUIRE(merge_idx < 0);

	merge_subs.push_back(source_sub);
	const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);

	REQUIRE(patched.find("new_ability") != std::string::npos);
	REQUIRE(patched.find("Sign Name") != std::string::npos);
}

TEST_CASE("sub_record_merge_t, patch second occurrence by index", "[u]")
{
	auto merge_content = make_record("BSGN",
		make_sub("NAME", make_string("id")) +
		make_sub("NPCS", make_string("ability_a")) +
		make_sub("NPCS", make_string("ability_b")));

	auto source_content = make_record("BSGN",
		make_sub("NAME", make_string("id")) +
		make_sub("NPCS", make_string("ability_a")) +
		make_sub("NPCS", make_string("ability_c")));

	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);
	const auto source_subs = sub_record_merge_t::parse_sub_records(source_content);

	const size_t binary_idx = 2;
	const auto & source_sub = source_subs[binary_idx];

	int source_occurrence = 0;
	for (size_t s = 0; s < binary_idx; ++s)
	{
		if (source_subs[s].type == "NPCS")
			++source_occurrence;
	}
	REQUIRE(source_occurrence == 1);

	const auto merge_idx = sub_record_merge_t::find_by_type_and_occurrence(merge_subs, "NPCS", source_occurrence);
	REQUIRE(merge_idx == 2);

	merge_subs[merge_idx].data = source_sub.data;
	const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);

	REQUIRE(patched.find("ability_a") != std::string::npos);
	REQUIRE(patched.find("ability_c") != std::string::npos);
	REQUIRE(patched.find("ability_b") == std::string::npos);
}

// ============================================================================
// Binary index resolution (col_type_indices mapping)
// ============================================================================

TEST_CASE("sub_record_merge_t, binary index generic no reorder", "[u]")
{
	auto content = make_record("WEAP",
		make_sub("NAME", make_string("id")) +
		make_sub("FNAM", make_string("Sword")) +
		make_sub("ENAM", make_string("enchant")));

	const auto subs = sub_record_merge_t::parse_sub_records(content);

	std::unordered_map<std::string, std::vector<size_t>> type_indices;
	for (size_t i = 0; i < subs.size(); ++i)
		type_indices[subs[i].type].push_back(i);

	REQUIRE(type_indices["NAME"][0] == 0);
	REQUIRE(type_indices["FNAM"][0] == 1);
	REQUIRE(type_indices["ENAM"][0] == 2);
}

TEST_CASE("sub_record_merge_t, binary index multiple same type", "[u]")
{
	auto content = make_record("BSGN",
		make_sub("NAME", make_string("id")) +
		make_sub("NPCS", make_string("ability_a")) +
		make_sub("NPCS", make_string("ability_b")) +
		make_sub("NPCS", make_string("ability_c")));

	const auto subs = sub_record_merge_t::parse_sub_records(content);

	std::unordered_map<std::string, std::vector<size_t>> type_indices;
	for (size_t i = 0; i < subs.size(); ++i)
		type_indices[subs[i].type].push_back(i);

	REQUIRE(type_indices["NPCS"].size() == 3);
	REQUIRE(type_indices["NPCS"][0] == 1);
	REQUIRE(type_indices["NPCS"][1] == 2);
	REQUIRE(type_indices["NPCS"][2] == 3);

	REQUIRE(subs[type_indices["NPCS"][0]].data.find("ability_a") != std::string::npos);
	REQUIRE(subs[type_indices["NPCS"][1]].data.find("ability_b") != std::string::npos);
	REQUIRE(subs[type_indices["NPCS"][2]].data.find("ability_c") != std::string::npos);
}

TEST_CASE("sub_record_merge_t, patch by binary index not occurrence", "[u]")
{
	auto source_content = make_record("BSGN",
		make_sub("NAME", make_string("id")) +
		make_sub("NPCS", make_string("spell_x")) +
		make_sub("NPCS", make_string("spell_y")) +
		make_sub("NPCS", make_string("spell_z")));

	auto merge_content = make_record("BSGN",
		make_sub("NAME", make_string("id")) +
		make_sub("NPCS", make_string("spell_x")) +
		make_sub("NPCS", make_string("spell_old")) +
		make_sub("NPCS", make_string("spell_z")));

	const auto source_subs = sub_record_merge_t::parse_sub_records(source_content);
	auto merge_subs = sub_record_merge_t::parse_sub_records(merge_content);

	const size_t binary_idx = 2;
	const auto & source_sub = source_subs[binary_idx];

	REQUIRE(source_sub.data.find("spell_y") != std::string::npos);

	int source_occurrence = 0;
	for (size_t s = 0; s < binary_idx; ++s)
	{
		if (source_subs[s].type == source_sub.type)
			++source_occurrence;
	}

	const auto merge_idx = sub_record_merge_t::find_by_type_and_occurrence(
		merge_subs, source_sub.type, source_occurrence);
	REQUIRE(merge_idx == 2);

	merge_subs[merge_idx].data = source_sub.data;
	const auto patched = sub_record_merge_t::reconstruct_record(merge_content, merge_subs);

	REQUIRE(patched.find("spell_x") != std::string::npos);
	REQUIRE(patched.find("spell_y") != std::string::npos);
	REQUIRE(patched.find("spell_z") != std::string::npos);
	REQUIRE(patched.find("spell_old") == std::string::npos);
}

TEST_CASE("sub_record_merge_t, reordered list binary index", "[u]")
{
	auto content = make_record("LEVC",
		make_sub("CNAM", make_string("skeleton")) +
		make_sub("INTV", make_uint16(3)) +
		make_sub("CNAM", make_string("ancestor_ghost")) +
		make_sub("INTV", make_uint16(1)) +
		make_sub("CNAM", make_string("rat")) +
		make_sub("INTV", make_uint16(1)));

	const auto subs = sub_record_merge_t::parse_sub_records(content);

	std::unordered_map<std::string, std::vector<size_t>> type_indices;
	for (size_t i = 0; i < subs.size(); ++i)
		type_indices[subs[i].type].push_back(i);

	REQUIRE(type_indices["CNAM"][0] == 0);
	REQUIRE(type_indices["CNAM"][1] == 2);
	REQUIRE(type_indices["CNAM"][2] == 4);

	REQUIRE(subs[type_indices["CNAM"][0]].data.find("skeleton") != std::string::npos);
	REQUIRE(subs[type_indices["CNAM"][1]].data.find("ancestor_ghost") != std::string::npos);
	REQUIRE(subs[type_indices["CNAM"][2]].data.find("rat") != std::string::npos);

	size_t aligned_view_occurrence = 1;
	size_t binary_idx_from_view = type_indices["CNAM"][aligned_view_occurrence];
	REQUIRE(binary_idx_from_view == 2);
	REQUIRE(subs[binary_idx_from_view].data.find("ancestor_ghost") != std::string::npos);
}

// ============================================================================
// SCPT records skipped from merge
// ============================================================================

TEST_CASE("sub_record_merge_t::merge, SCPT returns winner unchanged", "[u]")
{
	auto subs_first = make_sub("SCHD", make_bytes({23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x72, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));
	auto subs_inter = make_sub("SCHD", make_bytes({23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x74, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));
	auto subs_winner = make_sub("SCHD", make_bytes({23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x72, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}));

	auto first = make_record("SCPT", subs_first);
	auto inter = make_record("SCPT", subs_inter);
	auto winner = make_record("SCPT", subs_winner);

	merge_input_t input;
	input.rec_type = "SCPT";
	input.record_id = "TestScript";
	input.version_contents = { first, inter, winner };

	auto result = sub_record_merge_t::merge(input);

	REQUIRE_FALSE(result.changed);
	REQUIRE(result.content == winner);
}

// ============================================================================
// AIDT element-wise merge
// ============================================================================

TEST_CASE("sub_record_merge_t::merge, AIDT element-wise fight byte", "[u]")
{
	std::string aidt_first(12, '\0');
	aidt_first[1] = 90;

	std::string aidt_inter(12, '\0');
	aidt_inter[1] = 85;

	std::string aidt_winner(12, '\0');
	aidt_winner[1] = 90;
	aidt_winner[5] = 1;

	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("AIDT", aidt_first);
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("AIDT", aidt_inter);
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("AIDT", aidt_winner);

	merge_input_t input;
	input.rec_type = "CREA";
	input.record_id = "id";
	input.version_contents = {
		make_record("CREA", subs_first),
		make_record("CREA", subs_inter),
		make_record("CREA", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	auto result_subs = sub_record_merge_t::parse_sub_records(result.content);
	auto aidt_idx = sub_record_merge_t::find_by_type_and_occurrence(result_subs, "AIDT", 0);
	REQUIRE(aidt_idx >= 0);
	REQUIRE(static_cast<uint8_t>(result_subs[aidt_idx].data[1]) == 85);
	REQUIRE(static_cast<uint8_t>(result_subs[aidt_idx].data[5]) == 1);
}

// ============================================================================
// ENAM magnitude pair fix
// ============================================================================

TEST_CASE("sub_record_merge_t::merge, ENAM mag min/max from same source", "[u]")
{
	auto enam_first = make_enam(79, 0, 0, 10, 40, 60);
	auto enam_inter = make_enam(79, 0, 0, 10, 40, 40);
	auto enam_winner = make_enam(79, 0, 0, 10, 60, 60);

	merge_input_t input;
	input.rec_type = "SPEL";
	input.record_id = "id";
	input.version_contents = {
		make_record("SPEL", make_sub("NAME", make_string("id")) + make_sub("ENAM", enam_first)),
		make_record("SPEL", make_sub("NAME", make_string("id")) + make_sub("ENAM", enam_inter)),
		make_record("SPEL", make_sub("NAME", make_string("id")) + make_sub("ENAM", enam_winner)),
	};

	auto result = sub_record_merge_t::merge(input);

	auto result_subs = sub_record_merge_t::parse_sub_records(result.content);
	auto enam_idx = sub_record_merge_t::find_by_type_and_occurrence(result_subs, "ENAM", 0);
	REQUIRE(enam_idx >= 0);

	int32_t min_mag = 0;
	int32_t max_mag = 0;
	std::memcpy(&min_mag, result_subs[enam_idx].data.data() + 12, 4);
	std::memcpy(&max_mag, result_subs[enam_idx].data.data() + 16, 4);

	REQUIRE(min_mag == max_mag);
	REQUIRE((min_mag == 60 || min_mag == 40));
}

// ============================================================================
// CREA NPDT attack pair fix
// ============================================================================

TEST_CASE("sub_record_merge_t::merge, CREA attack min/max paired", "[u]")
{
	std::string npdt_first(96, '\0');
	npdt_first[68] = 2;
	npdt_first[70] = 6;

	std::string npdt_inter(96, '\0');
	npdt_inter[68] = 5;
	npdt_inter[70] = 10;

	std::string npdt_winner(96, '\0');
	npdt_winner[68] = 2;
	npdt_winner[70] = 6;
	npdt_winner[4] = 10;

	auto subs_first = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_first);
	auto subs_inter = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_inter);
	auto subs_winner = make_sub("NAME", make_string("id")) + make_sub("NPDT", npdt_winner);

	merge_input_t input;
	input.rec_type = "CREA";
	input.record_id = "id";
	input.version_contents = {
		make_record("CREA", subs_first),
		make_record("CREA", subs_inter),
		make_record("CREA", subs_winner),
	};

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	auto result_subs = sub_record_merge_t::parse_sub_records(result.content);
	auto npdt_idx = sub_record_merge_t::find_by_type_and_occurrence(result_subs, "NPDT", 0);
	REQUIRE(npdt_idx >= 0);

	uint8_t atk1_min = static_cast<uint8_t>(result_subs[npdt_idx].data[68]);
	uint8_t atk1_max = static_cast<uint8_t>(result_subs[npdt_idx].data[70]);

	REQUIRE(atk1_min == 5);
	REQUIRE(atk1_max == 10);
}

// ============================================================================
// Leveled list merge LEVC
// ============================================================================

TEST_CASE("leveled_list_merge_t::merge, LEVC creature list", "[u]")
{
	auto make_levc_entry = [](const std::string & creature, uint16_t level) {
		std::string cnam_data = creature;
		cnam_data.push_back('\0');
		std::string intv_data(2, '\0');
		std::memcpy(intv_data.data(), &level, 2);
		return make_sub("CNAM", cnam_data) + make_sub("INTV", intv_data);
	};

	auto make_header = [](const std::string & name, uint32_t count) {
		std::string name_data = name + '\0';
		uint32_t flags = 1;
		uint8_t chance = 0;
		return make_sub("NAME", name_data) +
		       make_sub("DATA", make_uint32(flags)) +
		       make_sub("NNAM", std::string(1, static_cast<char>(chance)));
	};

	auto first_subs = make_header("list", 2) +
	                  make_levc_entry("rat", 1) +
	                  make_levc_entry("skeleton", 3);
	auto mod_subs = make_header("list", 3) +
	               make_levc_entry("rat", 1) +
	               make_levc_entry("skeleton", 3) +
	               make_levc_entry("wolf", 5);

	leveled_list_input_t input;
	input.rec_type = "LEVC";
	input.record_id = "list";
	input.version_contents = {
		make_record("LEVC", first_subs),
		make_record("LEVC", mod_subs),
	};

	auto result = leveled_list_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(result.content.find("rat") != std::string::npos);
	REQUIRE(result.content.find("skeleton") != std::string::npos);
	REQUIRE(result.content.find("wolf") != std::string::npos);
}
