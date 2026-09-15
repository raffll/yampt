#include <catch2/catch_all.hpp>
#include <io/binary_file_io.hpp>
#include <io/esm_reader.hpp>
#include <scanner/plugin_index.hpp>
#include <utility/app_logger.hpp>
#include <filesystem>

static std::string get_temp_path(const std::string & filename)
{
	return (std::filesystem::temp_directory_path() / filename).string();
}

static std::string make_sub_record(const std::string & sub_id, const std::string & data)
{
	std::string result;
	result += sub_id;
	result += domain_types::convert_uint_to_string_byte_array(data.size());
	result += data;
	return result;
}

static std::string make_record(const std::string & rec_id, const std::string & sub_records)
{
	std::string header;
	header += rec_id;
	header += domain_types::convert_uint_to_string_byte_array(sub_records.size());
	header += domain_types::convert_uint_to_string_byte_array(0);
	header += domain_types::convert_uint_to_string_byte_array(0);
	return header + sub_records;
}

static std::string make_tes3()
{
	return make_record("TES3", make_sub_record("HEDR", std::string(300, '\0')));
}

static std::string make_cell_data(uint32_t flags, int32_t grid_x, int32_t grid_y)
{
	std::string data(12, '\0');
	std::memcpy(data.data(), &flags, 4);
	std::memcpy(data.data() + 4, &grid_x, 4);
	std::memcpy(data.data() + 8, &grid_y, 4);
	return data;
}

static std::string make_grid_intv(int32_t grid_x, int32_t grid_y)
{
	std::string data(8, '\0');
	std::memcpy(data.data(), &grid_x, 4);
	std::memcpy(data.data() + 4, &grid_y, 4);
	return data;
}

static std::string make_indx_sub(int32_t index_value)
{
	std::string data(4, '\0');
	std::memcpy(data.data(), &index_value, 4);
	return make_sub_record("INDX", data);
}

static plugin_index_t build_index(const std::vector<std::string> & records, const std::string & filename)
{
	std::string content;
	for (const auto & record : records)
		content += record;

	const auto temp_path = get_temp_path(filename);
	binary_file_io::write_text(content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	REQUIRE(reader.is_loaded());
	return plugin_index_t(reader);
}

TEST_CASE("plugin_index_t::plugin_index_t, interior cell keyed by NAME", "[i]")
{
	auto cell = make_record(
	    "CELL",
	    make_sub_record("NAME", std::string("Balmora, Guild of Mages\0", 24)) +
	        make_sub_record("DATA", make_cell_data(0x01, 0, 0)));

	const auto index = build_index({ make_tes3(), cell }, "yampt_pidx_cell_interior.esm");

	const auto * entry = index.find("CELL", "Balmora, Guild of Mages");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->record_id == "Balmora, Guild of Mages");
}

TEST_CASE("plugin_index_t::plugin_index_t, exterior cell keyed by grid", "[i]")
{
	auto cell = make_record(
	    "CELL", make_sub_record("NAME", std::string("\0", 1)) + make_sub_record("DATA", make_cell_data(0x00, 2, -3)));

	const auto index = build_index({ make_tes3(), cell }, "yampt_pidx_cell_exterior.esm");

	const auto * entry = index.find("CELL", "GRID[2,-3]");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->record_id == "GRID[2,-3]");
}

TEST_CASE("plugin_index_t::plugin_index_t, SKIL and MGEF keyed by INDX decimal", "[i]")
{
	auto skil = make_record("SKIL", make_indx_sub(11) + make_sub_record("SKDT", std::string(24, '\0')));
	auto mgef = make_record("MGEF", make_indx_sub(79) + make_sub_record("MEDT", std::string(36, '\0')));

	const auto index = build_index({ make_tes3(), skil, mgef }, "yampt_pidx_indx.esm");

	REQUIRE(index.find("SKIL", "11") != nullptr);
	REQUIRE(index.find("MGEF", "79") != nullptr);
}

TEST_CASE("plugin_index_t::plugin_index_t, SCPT keyed by SCHD name", "[i]")
{
	std::string schd(52, '\0');
	const std::string script_name = "AbebaalAttack";
	std::memcpy(schd.data(), script_name.data(), script_name.size());

	auto scpt = make_record("SCPT", make_sub_record("SCHD", schd));

	const auto index = build_index({ make_tes3(), scpt }, "yampt_pidx_scpt.esm");

	const auto * entry = index.find("SCPT", "AbebaalAttack");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->record_id == "AbebaalAttack");
}

TEST_CASE("plugin_index_t::plugin_index_t, LAND and PGRD keyed by grid", "[i]")
{
	auto land = make_record("LAND", make_sub_record("INTV", make_grid_intv(5, 7)));
	auto pgrd = make_record("PGRD", make_sub_record("DATA", make_grid_intv(-1, 4)));

	const auto index = build_index({ make_tes3(), land, pgrd }, "yampt_pidx_grid.esm");

	REQUIRE(index.find("LAND", "GRID[5,7]") != nullptr);
	REQUIRE(index.find("PGRD", "GRID[-1,4]") != nullptr);
}

TEST_CASE("plugin_index_t::plugin_index_t, INFO record keyed by dial and INAM", "[i]")
{
	auto dial = make_record("DIAL", make_sub_record("NAME", std::string("Greeting 0\0", 11)));
	auto info = make_record(
	    "INFO",
	    make_sub_record("INAM", std::string("info_001\0", 9)) + make_sub_record("NAME", std::string("Hello.\0", 7)));

	const auto index = build_index({ make_tes3(), dial, info }, "yampt_pidx_info.esm");

	const auto * entry = index.find("INFO", "Greeting 0|info_001");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->dial_name == "Greeting 0");
}

TEST_CASE("plugin_index_t::plugin_index_t, unknown record type falls back to NAME", "[i]")
{
	auto weap = make_record(
	    "WEAP",
	    make_sub_record("NAME", std::string("iron_dagger\0", 12)) + make_sub_record("WPDT", std::string(32, '\0')));

	const auto index = build_index({ make_tes3(), weap }, "yampt_pidx_default_name.esm");

	const auto * entry = index.find("WEAP", "iron_dagger");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->record_id == "iron_dagger");
}

TEST_CASE("plugin_index_t::plugin_index_t, record without id sub-record keys on index string", "[i]")
{
	auto weap = make_record("WEAP", make_sub_record("WPDT", std::string(32, '\0')));

	const auto index = build_index({ make_tes3(), weap }, "yampt_pidx_index_fallback.esm");

	const auto * entry = index.find("WEAP", "1");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->record_id == "1");
}

TEST_CASE("plugin_index_t::plugin_index_t, DELE sub-record marks record deleted", "[i]")
{
	auto weap = make_record(
	    "WEAP", make_sub_record("NAME", std::string("gone_weapon\0", 12)) + make_sub_record("DELE", std::string(4, '\0')));

	const auto index = build_index({ make_tes3(), weap }, "yampt_pidx_dele.esm");

	const auto * entry = index.find("WEAP", "gone_weapon");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->has_dele);
}

TEST_CASE("plugin_index_t::plugin_index_t, first occurrence wins on duplicate key", "[i]")
{
	auto weap_a = make_record("WEAP", make_sub_record("NAME", std::string("dup\0", 4)) + make_sub_record("WPDT", std::string("first", 5)));
	auto weap_b = make_record("WEAP", make_sub_record("NAME", std::string("dup\0", 4)) + make_sub_record("WPDT", std::string("second", 6)));

	const auto index = build_index({ make_tes3(), weap_a, weap_b }, "yampt_pidx_dup.esm");

	const auto * entry = index.find("WEAP", "dup");
	REQUIRE(entry != nullptr);
	REQUIRE(entry->record_index == 1);
}
