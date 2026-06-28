#include <catch2/catch_all.hpp>
#include "../yampt/model/dict_creator.hpp"

#include <cstring>
#include <filesystem>

static std::string get_temp_path(const std::string & filename)
{
	return (std::filesystem::temp_directory_path() / filename).string();
}

static std::string make_sub_record(const std::string & sub_id, const std::string & data)
{
	std::string result;
	result += sub_id;
	result += tools_t::convert_uint_to_string_byte_array(data.size());
	result += data;
	return result;
}

static std::string make_record(const std::string & rec_id, const std::string & sub_records)
{
	std::string header;
	header += rec_id;
	header += tools_t::convert_uint_to_string_byte_array(sub_records.size());
	header += tools_t::convert_uint_to_string_byte_array(0);
	header += tools_t::convert_uint_to_string_byte_array(0);
	return header + sub_records;
}

static std::string make_tes3_record()
{
	auto hedr_body = std::string(300, '\0');
	return make_record("TES3", make_sub_record("HEDR", hedr_body));
}

static std::string make_null_terminated(const std::string & text)
{
	return text + std::string(1, '\0');
}

static std::string make_dial_data_topic()
{
	return std::string(1, '\0');
}

static std::string make_cell_data(bool interior, int32_t grid_x = 0, int32_t grid_y = 0)
{
	std::string data(12, '\0');
	if (interior)
		data[0] = '\x01';
	else
		data[0] = '\x00';
	std::memcpy(data.data() + 4, &grid_x, 4);
	std::memcpy(data.data() + 8, &grid_y, 4);
	return data;
}

TEST_CASE("dict_creator_t, dial before info ordering", "[i]")
{
	auto native_dial_body = make_sub_record("DATA", make_dial_data_topic()) +
	                        make_sub_record("NAME", make_null_terminated("zwiadowca kwama"));
	auto native_dial = make_record("DIAL", native_dial_body);

	auto native_info_body = make_sub_record("INAM", make_null_terminated("12345")) +
	                        make_sub_record("NAME", make_null_terminated("To jest zwiadowca kwama."));
	auto native_info = make_record("INFO", native_info_body);

	auto foreign_dial_body = make_sub_record("DATA", make_dial_data_topic()) +
	                         make_sub_record("NAME", make_null_terminated("kwama forager"));
	auto foreign_dial = make_record("DIAL", foreign_dial_body);

	auto foreign_info_body = make_sub_record("INAM", make_null_terminated("12345")) +
	                         make_sub_record("NAME", make_null_terminated("This is a kwama forager."));
	auto foreign_info = make_record("INFO", foreign_info_body);

	auto dummy_gmst_body =
	    make_sub_record("NAME", make_null_terminated("iDummy")) + make_sub_record("INTV", std::string(4, '\0'));
	auto dummy_gmst = make_record("GMST", dummy_gmst_body);

	auto native_content = make_tes3_record() + native_dial + native_info;
	auto foreign_content = make_tes3_record() + dummy_gmst + foreign_dial + foreign_info;

	const auto native_path = get_temp_path("yampt_test_dial_order_native.esm");
	const auto foreign_path = get_temp_path("yampt_test_dial_order_foreign.esm");
	tools_t::write_text(native_content, native_path);
	tools_t::write_text(foreign_content, foreign_path);

	dict_creator_t creator(foreign_path, native_path);

	std::filesystem::remove(native_path);
	std::filesystem::remove(foreign_path);

	const auto & info_chapter = creator.get_dict().at(tools_t::rec_type_t::info);
	REQUIRE(info_chapter.size() == 1);

	const auto & entry = info_chapter.records[0];
	REQUIRE(entry.key_text.find("kwama forager") != std::string::npos);
	REQUIRE(entry.old_text == "This is a kwama forager.");
	REQUIRE(entry.new_text == "To jest zwiadowca kwama.");
}

TEST_CASE("dict_creator_t, cell matching sequence", "[i]")
{
	auto dodt_data = std::string(24, '\x01');
	auto frmr_data = tools_t::convert_uint_to_string_byte_array(1);

	auto native_exterior_body = make_sub_record("NAME", make_null_terminated("Poludniowy Mur")) +
	                            make_sub_record("DATA", make_cell_data(false, 5, -3));
	auto native_exterior = make_record("CELL", native_exterior_body);

	auto native_interior_body = make_sub_record("NAME", make_null_terminated("Jaskinia Addamasartus")) +
	                            make_sub_record("DATA", make_cell_data(true)) + make_sub_record("FRMR", frmr_data) +
	                            make_sub_record("NAME", make_null_terminated("barrel_01")) +
	                            make_sub_record("DODT", dodt_data);
	auto native_interior = make_record("CELL", native_interior_body);

	auto native_heuristic_body =
	    make_sub_record("NAME", make_null_terminated("Stara Kopalnia")) + make_sub_record("DATA", make_cell_data(true));
	auto native_heuristic = make_record("CELL", native_heuristic_body);

	auto foreign_exterior_body = make_sub_record("NAME", make_null_terminated("South Wall")) +
	                             make_sub_record("DATA", make_cell_data(false, 5, -3));
	auto foreign_exterior = make_record("CELL", foreign_exterior_body);

	auto foreign_interior_body = make_sub_record("NAME", make_null_terminated("Addamasartus")) +
	                             make_sub_record("DATA", make_cell_data(true)) + make_sub_record("FRMR", frmr_data) +
	                             make_sub_record("NAME", make_null_terminated("barrel_01")) +
	                             make_sub_record("DODT", dodt_data);
	auto foreign_interior = make_record("CELL", foreign_interior_body);

	auto foreign_heuristic_body =
	    make_sub_record("NAME", make_null_terminated("Old Mine")) + make_sub_record("DATA", make_cell_data(true));
	auto foreign_heuristic = make_record("CELL", foreign_heuristic_body);

	auto dummy_gmst_body =
	    make_sub_record("NAME", make_null_terminated("iDummy")) + make_sub_record("INTV", std::string(4, '\0'));
	auto dummy_gmst = make_record("GMST", dummy_gmst_body);

	auto native_content = make_tes3_record() + native_exterior + native_interior + native_heuristic;
	auto foreign_content = make_tes3_record() + dummy_gmst + foreign_exterior + foreign_interior + foreign_heuristic;

	const auto native_path = get_temp_path("yampt_test_cell_seq_native.esm");
	const auto foreign_path = get_temp_path("yampt_test_cell_seq_foreign.esm");
	tools_t::write_text(native_content, native_path);
	tools_t::write_text(foreign_content, foreign_path);

	dict_creator_t creator(foreign_path, native_path);

	std::filesystem::remove(native_path);
	std::filesystem::remove(foreign_path);

	const auto & cell_chapter = creator.get_dict().at(tools_t::rec_type_t::cell);

	const auto * ptr_exterior = cell_chapter.find_by_old_text("South Wall");
	REQUIRE(ptr_exterior != nullptr);
	REQUIRE(ptr_exterior->new_text == "Poludniowy Mur");
	REQUIRE(ptr_exterior->status == "translated");

	const auto * ptr_interior = cell_chapter.find_by_old_text("Addamasartus");
	REQUIRE(ptr_interior != nullptr);
	REQUIRE(ptr_interior->new_text == "Jaskinia Addamasartus");
	REQUIRE(ptr_interior->status == "translated");

	const auto * ptr_heuristic = cell_chapter.find_by_old_text("Old Mine");
	REQUIRE(ptr_heuristic != nullptr);
	REQUIRE(ptr_heuristic->status == "missing");
}
