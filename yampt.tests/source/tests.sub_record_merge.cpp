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

static std::string make_uint32(uint32_t value)
{
	std::string result(4, '\0');
	std::memcpy(result.data(), &value, 4);
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
