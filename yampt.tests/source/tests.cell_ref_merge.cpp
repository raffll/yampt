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

static std::string make_cell_data(uint32_t cell_flags, int32_t grid_x, int32_t grid_y)
{
	std::string result(12, '\0');
	std::memcpy(result.data(), &cell_flags, 4);
	std::memcpy(result.data() + 4, &grid_x, 4);
	std::memcpy(result.data() + 8, &grid_y, 4);
	return result;
}

static std::string make_ref_position(float x_pos, float y_pos, float z_pos)
{
	std::string result(24, '\0');
	std::memcpy(result.data(), &x_pos, 4);
	std::memcpy(result.data() + 4, &y_pos, 4);
	std::memcpy(result.data() + 8, &z_pos, 4);
	return result;
}

static std::string make_frmr_group(uint32_t frmr_index, const std::string & object_id, float x_pos, float y_pos, float z_pos)
{
	return make_sub("FRMR", make_uint32(frmr_index))
	     + make_sub("NAME", make_string(object_id))
	     + make_sub("DATA", make_ref_position(x_pos, y_pos, z_pos));
}

static std::string make_cell_header(const std::string & cell_name, uint32_t cell_flags, int32_t grid_x, int32_t grid_y)
{
	return make_sub("NAME", make_string(cell_name))
	     + make_sub("DATA", make_cell_data(cell_flags, grid_x, grid_y));
}

// ============================================================================
// Requirement 14: Cell Reference Merging
// ============================================================================

TEST_CASE("cell_ref_merge [DISABLED], intermediate adds new FRMR preserved", "[.][u]")
{
	auto cell_hdr = make_cell_header("TestCell", 0x01, 0, 0);
	auto ref1 = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);
	auto ref2 = make_frmr_group(2, "chair_01", 300.0f, 400.0f, 0.0f);

	auto first = make_record("CELL", cell_hdr + ref1);
	auto inter = make_record("CELL", cell_hdr + ref1 + ref2);
	auto winner = make_record("CELL", cell_hdr + ref1);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = { first, inter, winner };

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(result.content.find("barrel_01") != std::string::npos);
	REQUIRE(result.content.find("chair_01") != std::string::npos);
}

TEST_CASE("cell_ref_merge [DISABLED], intermediate modifies ref winner unchanged", "[.][u]")
{
	auto cell_hdr = make_cell_header("TestCell", 0x01, 0, 0);
	auto ref1_orig = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);
	auto ref1_moved = make_frmr_group(1, "barrel_01", 500.0f, 600.0f, 0.0f);

	auto first = make_record("CELL", cell_hdr + ref1_orig);
	auto inter = make_record("CELL", cell_hdr + ref1_moved);
	auto winner = make_record("CELL", cell_hdr + ref1_orig);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = { first, inter, winner };

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	float x_pos = 0.0f;
	size_t frmr_pos = result.content.find("FRMR");
	REQUIRE(frmr_pos != std::string::npos);
	size_t data_pos = result.content.find("DATA", frmr_pos + 4);
	REQUIRE(data_pos != std::string::npos);
	std::memcpy(&x_pos, result.content.data() + data_pos + 8, 4);
	REQUIRE(x_pos == Catch::Approx(500.0f));
}

TEST_CASE("cell_ref_merge [DISABLED], winner removes ref removal stands", "[.][u]")
{
	auto cell_hdr = make_cell_header("TestCell", 0x01, 0, 0);
	auto ref1 = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);
	auto ref2 = make_frmr_group(2, "chair_01", 300.0f, 400.0f, 0.0f);
	auto ref2_mod = make_frmr_group(2, "chair_01", 999.0f, 999.0f, 0.0f);

	auto first = make_record("CELL", cell_hdr + ref1 + ref2);
	auto inter = make_record("CELL", cell_hdr + ref1 + ref2_mod);
	auto winner = make_record("CELL", cell_hdr + ref1);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = { first, inter, winner };

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.content.find("barrel_01") != std::string::npos);
	REQUIRE(result.content.find("chair_01") == std::string::npos);
}

TEST_CASE("cell_ref_merge [DISABLED], both modify same ref winner wins", "[.][u]")
{
	auto cell_hdr = make_cell_header("TestCell", 0x01, 0, 0);
	auto ref1_orig = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);
	auto ref1_inter = make_frmr_group(1, "barrel_01", 500.0f, 500.0f, 0.0f);
	auto ref1_winner = make_frmr_group(1, "barrel_01", 900.0f, 900.0f, 0.0f);

	auto first = make_record("CELL", cell_hdr + ref1_orig);
	auto inter = make_record("CELL", cell_hdr + ref1_inter);
	auto winner = make_record("CELL", cell_hdr + ref1_winner);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = { first, inter, winner };

	auto result = sub_record_merge_t::merge(input);

	REQUIRE_FALSE(result.changed);
	REQUIRE(result.content == winner);
}

TEST_CASE("cell_ref_merge [DISABLED], header sub-records merged alongside refs", "[.][u]")
{
	std::string ambi_first(16, '\0');
	ambi_first[0] = 50;

	std::string ambi_inter(16, '\0');
	ambi_inter[0] = 100;

	auto cell_hdr_first = make_cell_header("TestCell", 0x01, 0, 0) + make_sub("AMBI", ambi_first);
	auto cell_hdr_inter = make_cell_header("TestCell", 0x01, 0, 0) + make_sub("AMBI", ambi_inter);
	auto cell_hdr_winner = make_cell_header("TestCell", 0x01, 0, 0) + make_sub("AMBI", ambi_first);

	auto ref1 = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);
	auto ref2 = make_frmr_group(2, "chair_01", 300.0f, 400.0f, 0.0f);

	auto first = make_record("CELL", cell_hdr_first + ref1);
	auto inter = make_record("CELL", cell_hdr_inter + ref1 + ref2);
	auto winner = make_record("CELL", cell_hdr_winner + ref1);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = { first, inter, winner };

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(result.content.find("chair_01") != std::string::npos);
	size_t ambi_pos = result.content.find("AMBI");
	REQUIRE(ambi_pos != std::string::npos);
	REQUIRE(static_cast<uint8_t>(result.content[ambi_pos + 8]) == 100);
}

TEST_CASE("cell_ref_merge [DISABLED], two intermediates add different refs", "[.][u]")
{
	auto cell_hdr = make_cell_header("TestCell", 0x01, 0, 0);
	auto ref1 = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);
	auto ref2 = make_frmr_group(2, "chair_01", 300.0f, 400.0f, 0.0f);
	auto ref3 = make_frmr_group(3, "table_01", 500.0f, 600.0f, 0.0f);

	auto first = make_record("CELL", cell_hdr + ref1);
	auto inter1 = make_record("CELL", cell_hdr + ref1 + ref2);
	auto inter2 = make_record("CELL", cell_hdr + ref1 + ref3);
	auto winner = make_record("CELL", cell_hdr + ref1);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = { first, inter1, inter2, winner };

	auto result = sub_record_merge_t::merge(input);

	REQUIRE(result.changed);
	REQUIRE(result.content.find("barrel_01") != std::string::npos);
	REQUIRE(result.content.find("chair_01") != std::string::npos);
	REQUIRE(result.content.find("table_01") != std::string::npos);
}
