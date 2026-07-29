#include <catch2/catch_all.hpp>
#include <scanner/header_repair.hpp>
#include <utility/domain_types.hpp>
#include <cstring>

static std::string make_sub_record(const std::string & type, const std::string & data)
{
	return type + domain_types::convert_uint_to_string_byte_array(data.size()) + data;
}

static std::string make_hedr(float version)
{
	std::string data(300, '\0');
	std::memcpy(data.data(), &version, sizeof(float));
	return make_sub_record("HEDR", data);
}

static std::string make_mast(const std::string & filename)
{
	auto name_with_null = filename + '\0';
	return make_sub_record("MAST", name_with_null);
}

static std::string make_data_size(uint64_t file_size)
{
	std::string data(8, '\0');
	std::memcpy(data.data(), &file_size, sizeof(uint64_t));
	return make_sub_record("DATA", data);
}

static std::string make_record_header(size_t body_size)
{
	std::string header(16, '\0');
	header[0] = 'T';
	header[1] = 'E';
	header[2] = 'S';
	header[3] = '3';
	auto size_bytes = domain_types::convert_uint_to_string_byte_array(body_size);
	header.replace(4, 4, size_bytes);
	return header;
}

static std::string build_tes3_record(const std::string & body)
{
	return make_record_header(body.size()) + body;
}

static float read_version(const std::string & content)
{
	size_t scan_pos = 16;
	while (scan_pos + 8 <= content.size())
	{
		auto sub_type = content.substr(scan_pos, 4);
		auto sub_size = domain_types::convert_string_byte_array_to_uint(content.substr(scan_pos + 4, 4));
		if (sub_type == "HEDR" && sub_size >= 4)
		{
			float version = 0.0f;
			std::memcpy(&version, content.data() + scan_pos + 8, sizeof(float));
			return version;
		}
		scan_pos += 8 + sub_size;
	}
	return 0.0f;
}

static uint64_t read_master_data_size(const std::string & content, size_t master_index)
{
	size_t scan_pos = 16;
	size_t mast_count = 0;
	while (scan_pos + 8 <= content.size())
	{
		auto sub_type = content.substr(scan_pos, 4);
		auto sub_size = domain_types::convert_string_byte_array_to_uint(content.substr(scan_pos + 4, 4));
		if (sub_type == "MAST")
		{
			if (mast_count == master_index)
			{
				auto data_pos = scan_pos + 8 + sub_size;
				if (data_pos + 8 <= content.size() && content.substr(data_pos, 4) == "DATA")
				{
					uint64_t value = 0;
					std::memcpy(&value, content.data() + data_pos + 8, sizeof(uint64_t));
					return value;
				}
			}
			++mast_count;
		}
		scan_pos += 8 + sub_size;
	}
	return 0;
}

TEST_CASE("header_repair_t::update_version_to_1_3, sets version float", "[u]")
{
	auto body = make_hedr(1.2f);
	auto record = build_tes3_record(body);

	auto result = header_repair_t::update_version_to_1_3(record);

	REQUIRE(Catch::Approx(read_version(result)).epsilon(0.001) == 1.3f);
}

TEST_CASE("header_repair_t::update_version_to_1_3, already 1.3 unchanged", "[u]")
{
	auto body = make_hedr(1.3f);
	auto record = build_tes3_record(body);

	auto result = header_repair_t::update_version_to_1_3(record);

	REQUIRE(result == record);
}

TEST_CASE("header_repair_t::update_version_to_1_3, no HEDR unchanged", "[u]")
{
	auto body = make_sub_record("MAST", "Morrowind.esm\0");
	auto record = build_tes3_record(body);

	auto result = header_repair_t::update_version_to_1_3(record);

	REQUIRE(result == record);
}

TEST_CASE("header_repair_t::update_version_to_1_3, preserves other sub-records", "[u]")
{
	auto body = make_hedr(1.0f) + make_mast("Morrowind.esm") + make_data_size(5000);
	auto record = build_tes3_record(body);

	auto result = header_repair_t::update_version_to_1_3(record);

	REQUIRE(result.size() == record.size());
	REQUIRE(read_master_data_size(result, 0) == 5000);
}

TEST_CASE("header_repair_t::update_master_sizes, no masters unchanged", "[u]")
{
	auto body = make_hedr(1.3f);
	auto record = build_tes3_record(body);

	auto result = header_repair_t::update_master_sizes(record, "nonexistent_dir");

	REQUIRE(result == record);
}

TEST_CASE("header_repair_t::update_master_sizes, missing file leaves size unchanged", "[u]")
{
	auto body = make_hedr(1.3f) + make_mast("MissingFile.esm") + make_data_size(9999);
	auto record = build_tes3_record(body);

	auto result = header_repair_t::update_master_sizes(record, "nonexistent_dir");

	REQUIRE(read_master_data_size(result, 0) == 9999);
}

TEST_CASE("header_repair_t::update_master_sizes, multiple masters independent", "[u]")
{
	auto body = make_hedr(1.3f) + make_mast("A.esm") + make_data_size(100) + make_mast("B.esm") + make_data_size(200);
	auto record = build_tes3_record(body);

	auto result = header_repair_t::update_master_sizes(record, "nonexistent_dir");

	REQUIRE(read_master_data_size(result, 0) == 100);
	REQUIRE(read_master_data_size(result, 1) == 200);
}
