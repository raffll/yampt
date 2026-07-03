#include <catch2/catch_all.hpp>
#include <scanner/sub_record_merge.hpp>
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

static std::string make_cell_data(uint32_t flags, int32_t grid_x, int32_t grid_y)
{
	std::string result(12, '\0');
	std::memcpy(result.data(), &flags, 4);
	std::memcpy(result.data() + 4, &grid_x, 4);
	std::memcpy(result.data() + 8, &grid_y, 4);
	return result;
}

static std::string make_position(float x_pos, float y_pos, float z_pos)
{
	std::string result(24, '\0');
	std::memcpy(result.data(), &x_pos, 4);
	std::memcpy(result.data() + 4, &y_pos, 4);
	std::memcpy(result.data() + 8, &z_pos, 4);
	return result;
}

static std::string make_frmr_group(uint32_t index, const std::string & object_id, float x_pos, float y_pos, float z_pos)
{
	return make_sub("FRMR", make_uint32(index))
	     + make_sub("NAME", make_string(object_id))
	     + make_sub("DATA", make_position(x_pos, y_pos, z_pos));
}

static std::string make_cell_header()
{
	return make_sub("NAME", make_string("TestCell"))
	     + make_sub("DATA", make_cell_data(0x01, 0, 0));
}

static bool contains_frmr(const std::string & content, uint32_t frmr_index)
{
	auto part = sub_record_merge_t::partition_cell(content);
	for (const auto & group : part.groups)
	{
		if (group.frmr_index == frmr_index)
			return true;
	}

	return false;
}

static float read_frmr_x_pos(const std::string & content, uint32_t frmr_index)
{
	auto part = sub_record_merge_t::partition_cell(content);
	for (const auto & group : part.groups)
	{
		if (group.frmr_index != frmr_index)
			continue;

		for (const auto & entry : group.sub_records)
		{
			if (entry.type == "DATA" && entry.data.size() >= 4)
			{
				float value = 0.0f;
				std::memcpy(&value, entry.data.data(), 4);
				return value;
			}
		}
	}

	return 0.0f;
}

TEST_CASE("sub_record_merge_t::partition_cell, splits header from refs", "[u]")
{
	auto hdr = make_cell_header();
	auto ref1 = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);
	auto ref2 = make_frmr_group(2, "chair_01", 300.0f, 400.0f, 0.0f);

	auto content = make_record("CELL", hdr + ref1 + ref2);
	auto part = sub_record_merge_t::partition_cell(content);

	REQUIRE(part.header.size() == 2);
	REQUIRE(part.header[0].type == "NAME");
	REQUIRE(part.header[1].type == "DATA");
	REQUIRE(part.groups.size() == 2);
	REQUIRE(part.groups[0].frmr_index == 1);
	REQUIRE(part.groups[1].frmr_index == 2);
}

TEST_CASE("sub_record_merge_t::partition_cell, frmr group collects sub-records", "[u]")
{
	auto hdr = make_cell_header();
	auto ref = make_frmr_group(5, "item_05", 10.0f, 20.0f, 30.0f);

	auto content = make_record("CELL", hdr + ref);
	auto part = sub_record_merge_t::partition_cell(content);

	REQUIRE(part.groups.size() == 1);
	REQUIRE(part.groups[0].frmr_index == 5);
	REQUIRE(part.groups[0].sub_records.size() == 3);
	REQUIRE(part.groups[0].sub_records[0].type == "FRMR");
	REQUIRE(part.groups[0].sub_records[1].type == "NAME");
	REQUIRE(part.groups[0].sub_records[2].type == "DATA");
}

TEST_CASE("sub_record_merge_t::partition_cell, empty cell has no groups", "[u]")
{
	auto hdr = make_cell_header();
	auto content = make_record("CELL", hdr);
	auto part = sub_record_merge_t::partition_cell(content);

	REQUIRE(part.header.size() == 2);
	REQUIRE(part.groups.empty());
}

TEST_CASE("sub_record_merge_t::build_frmr_map, indexes by frmr_index", "[u]")
{
	auto hdr = make_cell_header();
	auto ref1 = make_frmr_group(10, "barrel", 0.0f, 0.0f, 0.0f);
	auto ref2 = make_frmr_group(20, "chair", 0.0f, 0.0f, 0.0f);
	auto ref3 = make_frmr_group(5, "table", 0.0f, 0.0f, 0.0f);

	auto content = make_record("CELL", hdr + ref1 + ref2 + ref3);
	auto part = sub_record_merge_t::partition_cell(content);
	auto frmr_map = sub_record_merge_t::build_frmr_map(part.groups);

	REQUIRE(frmr_map.size() == 3);
	REQUIRE(frmr_map.count(5) == 1);
	REQUIRE(frmr_map.count(10) == 1);
	REQUIRE(frmr_map.count(20) == 1);
}

TEST_CASE("sub_record_merge_t::read_frmr_index, reads uint32 from data", "[u]")
{
	sub_record_entry_t entry;
	entry.type = "FRMR";
	entry.data = make_uint32(42);

	REQUIRE(sub_record_merge_t::read_frmr_index(entry) == 42);
}

TEST_CASE("sub_record_merge_t::merge_cell_refs, 2 versions unchanged", "[u]")
{
	auto hdr = make_cell_header();
	auto ref1 = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = {
	    make_record("CELL", hdr + ref1),
	    make_record("CELL", hdr + ref1),
	};

	auto result = sub_record_merge_t::merge_cell_refs(input);

	REQUIRE_FALSE(result.changed);
}

TEST_CASE("sub_record_merge_t::merge_cell_refs, intermediate adds ref", "[u]")
{
	auto hdr = make_cell_header();
	auto ref1 = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);
	auto ref2 = make_frmr_group(2, "chair_01", 300.0f, 400.0f, 0.0f);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = {
	    make_record("CELL", hdr + ref1),
	    make_record("CELL", hdr + ref1 + ref2),
	    make_record("CELL", hdr + ref1),
	};

	auto result = sub_record_merge_t::merge_cell_refs(input);

	REQUIRE(result.changed);
	REQUIRE(contains_frmr(result.content, 1));
	REQUIRE(contains_frmr(result.content, 2));
}

TEST_CASE("sub_record_merge_t::merge_cell_refs, intermediate moves ref", "[u]")
{
	auto hdr = make_cell_header();
	auto ref1_orig = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);
	auto ref1_moved = make_frmr_group(1, "barrel_01", 500.0f, 600.0f, 0.0f);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = {
	    make_record("CELL", hdr + ref1_orig),
	    make_record("CELL", hdr + ref1_moved),
	    make_record("CELL", hdr + ref1_orig),
	};

	auto result = sub_record_merge_t::merge_cell_refs(input);

	REQUIRE(result.changed);
	REQUIRE(read_frmr_x_pos(result.content, 1) == Catch::Approx(500.0f));
}

TEST_CASE("sub_record_merge_t::merge_cell_refs, winner removes ref", "[u]")
{
	auto hdr = make_cell_header();
	auto ref1 = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);
	auto ref2 = make_frmr_group(2, "chair_01", 300.0f, 400.0f, 0.0f);
	auto ref2_moved = make_frmr_group(2, "chair_01", 999.0f, 999.0f, 0.0f);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = {
	    make_record("CELL", hdr + ref1 + ref2),
	    make_record("CELL", hdr + ref1 + ref2_moved),
	    make_record("CELL", hdr + ref1),
	};

	auto result = sub_record_merge_t::merge_cell_refs(input);

	REQUIRE_FALSE(result.changed);
	REQUIRE(contains_frmr(result.content, 1));
	REQUIRE_FALSE(contains_frmr(result.content, 2));
}

TEST_CASE("sub_record_merge_t::merge_cell_refs, both modify same ref winner wins", "[u]")
{
	auto hdr = make_cell_header();
	auto ref1_orig = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);
	auto ref1_inter = make_frmr_group(1, "barrel_01", 500.0f, 500.0f, 0.0f);
	auto ref1_winner = make_frmr_group(1, "barrel_01", 900.0f, 900.0f, 0.0f);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = {
	    make_record("CELL", hdr + ref1_orig),
	    make_record("CELL", hdr + ref1_inter),
	    make_record("CELL", hdr + ref1_winner),
	};

	auto result = sub_record_merge_t::merge_cell_refs(input);

	REQUIRE_FALSE(result.changed);
	REQUIRE(read_frmr_x_pos(result.content, 1) == Catch::Approx(900.0f));
}

TEST_CASE("sub_record_merge_t::merge_cell_refs, two intermediates add different refs", "[u]")
{
	auto hdr = make_cell_header();
	auto ref1 = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);
	auto ref2 = make_frmr_group(2, "chair_01", 300.0f, 400.0f, 0.0f);
	auto ref3 = make_frmr_group(3, "table_01", 500.0f, 600.0f, 0.0f);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = {
	    make_record("CELL", hdr + ref1),
	    make_record("CELL", hdr + ref1 + ref2),
	    make_record("CELL", hdr + ref1 + ref3),
	    make_record("CELL", hdr + ref1),
	};

	auto result = sub_record_merge_t::merge_cell_refs(input);

	REQUIRE(result.changed);
	REQUIRE(contains_frmr(result.content, 1));
	REQUIRE(contains_frmr(result.content, 2));
	REQUIRE(contains_frmr(result.content, 3));
}

TEST_CASE("sub_record_merge_t::merge_cell_refs, header merged alongside refs", "[u]")
{
	std::string ambi_first(16, '\0');
	ambi_first[0] = 50;

	std::string ambi_inter(16, '\0');
	ambi_inter[0] = 100;

	auto hdr_first = make_cell_header() + make_sub("AMBI", ambi_first);
	auto hdr_inter = make_cell_header() + make_sub("AMBI", ambi_inter);
	auto hdr_winner = make_cell_header() + make_sub("AMBI", ambi_first);

	auto ref1 = make_frmr_group(1, "barrel_01", 100.0f, 200.0f, 0.0f);
	auto ref2 = make_frmr_group(2, "chair_01", 300.0f, 400.0f, 0.0f);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = {
	    make_record("CELL", hdr_first + ref1),
	    make_record("CELL", hdr_inter + ref1 + ref2),
	    make_record("CELL", hdr_winner + ref1),
	};

	auto result = sub_record_merge_t::merge_cell_refs(input);

	REQUIRE(result.changed);
	REQUIRE(contains_frmr(result.content, 2));

	auto part = sub_record_merge_t::partition_cell(result.content);
	bool found_ambi = false;
	for (const auto & entry : part.header)
	{
		if (entry.type == "AMBI")
		{
			REQUIRE(static_cast<uint8_t>(entry.data[0]) == 100);
			found_ambi = true;
		}
	}
	REQUIRE(found_ambi);
}

TEST_CASE("sub_record_merge_t::merge_cell_refs, refs sorted by index", "[u]")
{
	auto hdr = make_cell_header();
	auto ref5 = make_frmr_group(5, "item_05", 0.0f, 0.0f, 0.0f);
	auto ref2 = make_frmr_group(2, "item_02", 0.0f, 0.0f, 0.0f);

	merge_input_t input;
	input.rec_type = "CELL";
	input.record_id = "TestCell";
	input.version_contents = {
	    make_record("CELL", hdr + ref5),
	    make_record("CELL", hdr + ref5 + ref2),
	    make_record("CELL", hdr + ref5),
	};

	auto result = sub_record_merge_t::merge_cell_refs(input);

	REQUIRE(result.changed);
	auto part = sub_record_merge_t::partition_cell(result.content);
	REQUIRE(part.groups.size() == 2);
	REQUIRE(part.groups[0].frmr_index == 2);
	REQUIRE(part.groups[1].frmr_index == 5);
}
