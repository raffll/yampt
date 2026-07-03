#include <catch2/catch_all.hpp>
#include <scanner/cell_name_fixer.hpp>
#include <scanner/fog_fixer.hpp>
#include <scanner/summon_fixer.hpp>
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

static std::string make_cell_data(uint32_t cell_flags, int32_t grid_x, int32_t grid_y)
{
	std::string result(12, '\0');
	std::memcpy(result.data(), &cell_flags, 4);
	std::memcpy(result.data() + 4, &grid_x, 4);
	std::memcpy(result.data() + 8, &grid_y, 4);
	return result;
}

static std::string make_ambi(float fog_density)
{
	std::string result(16, '\0');
	std::memcpy(result.data() + 12, &fog_density, 4);
	return result;
}

// ============================================================================
// Requirement 6: Fog Bug Fix
// ============================================================================

TEST_CASE("fog_fixer_t::apply, interior cell zero fog density gets fixed", "[u]")
{
	auto subs = make_sub("NAME", make_string("TestCell")) + make_sub("DATA", make_cell_data(0x01, 0, 0)) +
	            make_sub("AMBI", make_ambi(0.0f));

	auto content = make_record("CELL", subs);
	auto result = fog_fixer_t::apply(content);

	REQUIRE_FALSE(result.empty());
	float fixed_fog = 0.0f;
	size_t ambi_offset = content.find("AMBI");
	std::memcpy(&fixed_fog, result.data() + ambi_offset + 8 + 12, 4);
	REQUIRE(fixed_fog == Catch::Approx(0.01f));
}

TEST_CASE("fog_fixer_t::apply, exterior cell skipped", "[u]")
{
	auto subs = make_sub("NAME", make_string("")) + make_sub("DATA", make_cell_data(0x00, 5, 10)) +
	            make_sub("AMBI", make_ambi(0.0f));

	auto content = make_record("CELL", subs);
	auto result = fog_fixer_t::apply(content);

	REQUIRE(result.empty());
}

TEST_CASE("fog_fixer_t::apply, interior behave-exterior skipped", "[u]")
{
	auto subs = make_sub("NAME", make_string("TestCell")) + make_sub("DATA", make_cell_data(0x81, 0, 0)) +
	            make_sub("AMBI", make_ambi(0.0f));

	auto content = make_record("CELL", subs);
	auto result = fog_fixer_t::apply(content);

	REQUIRE(result.empty());
}

TEST_CASE("fog_fixer_t::apply, non-zero fog density skipped", "[u]")
{
	auto subs = make_sub("NAME", make_string("TestCell")) + make_sub("DATA", make_cell_data(0x01, 0, 0)) +
	            make_sub("AMBI", make_ambi(0.5f));

	auto content = make_record("CELL", subs);
	auto result = fog_fixer_t::apply(content);

	REQUIRE(result.empty());
}

TEST_CASE("fog_fixer_t::apply, interior no AMBI sub-record skipped", "[u]")
{
	auto subs = make_sub("NAME", make_string("TestCell")) + make_sub("DATA", make_cell_data(0x01, 0, 0));

	auto content = make_record("CELL", subs);
	auto result = fog_fixer_t::apply(content);

	REQUIRE(result.empty());
}

// ============================================================================
// Requirement 7: Summon Persist Fix
// ============================================================================

TEST_CASE("summon_fixer_t::apply, known summon without flag gets fixed", "[u]")
{
	auto subs = make_sub("NAME", make_string("clannfear_summon"));
	auto content = make_record("CREA", subs, 0x0000);

	auto result = summon_fixer_t::apply("clannfear_summon", content);

	REQUIRE_FALSE(result.empty());
	uint32_t output_flags = 0;
	std::memcpy(&output_flags, result.data() + 12, 4);
	REQUIRE((output_flags & 0x0400) != 0);
}

TEST_CASE("summon_fixer_t::apply, known summon case insensitive", "[u]")
{
	auto subs = make_sub("NAME", make_string("Clannfear_Summon"));
	auto content = make_record("CREA", subs, 0x0000);

	auto result = summon_fixer_t::apply("Clannfear_Summon", content);

	REQUIRE_FALSE(result.empty());
}

TEST_CASE("summon_fixer_t::apply, non-summon creature skipped", "[u]")
{
	auto subs = make_sub("NAME", make_string("rat"));
	auto content = make_record("CREA", subs, 0x0000);

	auto result = summon_fixer_t::apply("rat", content);

	REQUIRE(result.empty());
}

TEST_CASE("summon_fixer_t::apply, flag already set skipped", "[u]")
{
	auto subs = make_sub("NAME", make_string("scamp_summon"));
	auto content = make_record("CREA", subs, 0x0400);

	auto result = summon_fixer_t::apply("scamp_summon", content);

	REQUIRE(result.empty());
}

TEST_CASE("summon_fixer_t::apply, preserves existing flags", "[u]")
{
	auto subs = make_sub("NAME", make_string("dremora_summon"));
	auto content = make_record("CREA", subs, 0x0002);

	auto result = summon_fixer_t::apply("dremora_summon", content);

	REQUIRE_FALSE(result.empty());
	uint32_t output_flags = 0;
	std::memcpy(&output_flags, result.data() + 12, 4);
	REQUIRE(output_flags == 0x0402);
}

// ============================================================================
// Requirement 8: Cell Name Reversion Fix
// ============================================================================

TEST_CASE("cell_name_fixer_t::apply, winner reverts rename gets fixed", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("")) + make_sub("DATA", make_cell_data(0x00, 3, 5));
	auto subs_inter = make_sub("NAME", make_string("My Town")) + make_sub("DATA", make_cell_data(0x00, 3, 5));
	auto subs_winner = make_sub("NAME", make_string("")) + make_sub("DATA", make_cell_data(0x00, 3, 5));

	std::vector<std::string> versions = {
		make_record("CELL", subs_first),
		make_record("CELL", subs_inter),
		make_record("CELL", subs_winner),
	};

	auto result = cell_name_fixer_t::apply(versions);

	REQUIRE_FALSE(result.empty());
	REQUIRE(result.find("My Town") != std::string::npos);
}

TEST_CASE("cell_name_fixer_t::apply, interior cell skipped", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("Cave")) + make_sub("DATA", make_cell_data(0x01, 0, 0));
	auto subs_inter = make_sub("NAME", make_string("Renamed Cave")) + make_sub("DATA", make_cell_data(0x01, 0, 0));
	auto subs_winner = make_sub("NAME", make_string("Cave")) + make_sub("DATA", make_cell_data(0x01, 0, 0));

	std::vector<std::string> versions = {
		make_record("CELL", subs_first),
		make_record("CELL", subs_inter),
		make_record("CELL", subs_winner),
	};

	auto result = cell_name_fixer_t::apply(versions);

	REQUIRE(result.empty());
}

TEST_CASE("cell_name_fixer_t::apply, winner has own rename not reverted", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("")) + make_sub("DATA", make_cell_data(0x00, 1, 1));
	auto subs_inter = make_sub("NAME", make_string("Town A")) + make_sub("DATA", make_cell_data(0x00, 1, 1));
	auto subs_winner = make_sub("NAME", make_string("Town B")) + make_sub("DATA", make_cell_data(0x00, 1, 1));

	std::vector<std::string> versions = {
		make_record("CELL", subs_first),
		make_record("CELL", subs_inter),
		make_record("CELL", subs_winner),
	};

	auto result = cell_name_fixer_t::apply(versions);

	REQUIRE(result.empty());
}

TEST_CASE("cell_name_fixer_t::apply, multiple intermediates uses last", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("")) + make_sub("DATA", make_cell_data(0x00, 2, 2));
	auto subs_inter1 = make_sub("NAME", make_string("First Rename")) + make_sub("DATA", make_cell_data(0x00, 2, 2));
	auto subs_inter2 = make_sub("NAME", make_string("Second Rename")) + make_sub("DATA", make_cell_data(0x00, 2, 2));
	auto subs_winner = make_sub("NAME", make_string("")) + make_sub("DATA", make_cell_data(0x00, 2, 2));

	std::vector<std::string> versions = {
		make_record("CELL", subs_first),
		make_record("CELL", subs_inter1),
		make_record("CELL", subs_inter2),
		make_record("CELL", subs_winner),
	};

	auto result = cell_name_fixer_t::apply(versions);

	REQUIRE_FALSE(result.empty());
	REQUIRE(result.find("Second Rename") != std::string::npos);
	REQUIRE(result.find("First Rename") == std::string::npos);
}

TEST_CASE("cell_name_fixer_t::apply, only 2 versions skipped", "[u]")
{
	auto subs_first = make_sub("NAME", make_string("")) + make_sub("DATA", make_cell_data(0x00, 0, 0));
	auto subs_winner = make_sub("NAME", make_string("")) + make_sub("DATA", make_cell_data(0x00, 0, 0));

	std::vector<std::string> versions = {
		make_record("CELL", subs_first),
		make_record("CELL", subs_winner),
	};

	auto result = cell_name_fixer_t::apply(versions);

	REQUIRE(result.empty());
}
