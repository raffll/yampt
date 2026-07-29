#include <catch2/catch_all.hpp>
#include <io/binary_file_io.hpp>
#include <io/esm_reader.hpp>
#include <utility/app_logger.hpp>
#include <utility/includes.hpp>

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

static std::string make_esm_file(const std::vector<std::string> & records)
{
	std::string content;
	for (const auto & record : records)
		content += record;
	return content;
}

TEST_CASE("esm_reader_t::select_record, resets key and value", "[i]")
{
	auto tes3_body = make_sub_record("HEDR", std::string(300, '\0'));
	auto tes3 = make_record("TES3", tes3_body);

	auto gmst_body =
	    make_sub_record("NAME", std::string("sWelcome\0", 9)) + make_sub_record("STRV", std::string("Hello\0", 6));
	auto gmst = make_record("GMST", gmst_body);

	std::string file_content = tes3 + gmst;

	const auto temp_path = get_temp_path("yampt_test_select_record.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	REQUIRE(reader.is_loaded());

	reader.select_record(1);
	reader.set_key("NAME");
	REQUIRE(reader.get_key().exist == true);
	REQUIRE(reader.get_key().text == "sWelcome");

	reader.set_value("STRV");
	REQUIRE(reader.get_value().exist == true);
	REQUIRE(reader.get_value().text == "Hello");

	reader.select_record(0);
	REQUIRE(reader.get_key().exist == false);
	REQUIRE(reader.get_value().exist == false);
}

TEST_CASE("esm_reader_t::set_next_value, counter increments", "[i]")
{
	auto tes3_body = make_sub_record("HEDR", std::string(300, '\0'));
	auto tes3 = make_record("TES3", tes3_body);

	auto rnam1 = std::string("Rank One\0", 9);
	auto rnam2 = std::string("Rank Two\0", 9);
	auto rnam3 = std::string("Rank Three\0", 11);

	auto fact_body = make_sub_record("NAME", std::string("Fighters Guild\0", 15)) + make_sub_record("RNAM", rnam1) +
	                 make_sub_record("RNAM", rnam2) + make_sub_record("RNAM", rnam3);
	auto fact = make_record("FACT", fact_body);

	std::string file_content = tes3 + fact;

	const auto temp_path = get_temp_path("yampt_test_counter.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	REQUIRE(reader.is_loaded());

	reader.select_record(1);
	reader.set_value("RNAM");
	REQUIRE(reader.get_value().exist == true);
	REQUIRE(reader.get_value().text == "Rank One");
	REQUIRE(reader.get_value().counter == 0);

	reader.set_next_value("RNAM");
	REQUIRE(reader.get_value().exist == true);
	REQUIRE(reader.get_value().text == "Rank Two");
	REQUIRE(reader.get_value().counter == 1);

	reader.set_next_value("RNAM");
	REQUIRE(reader.get_value().exist == true);
	REQUIRE(reader.get_value().text == "Rank Three");
	REQUIRE(reader.get_value().counter == 2);

	reader.set_next_value("RNAM");
	REQUIRE(reader.get_value().exist == false);
}

TEST_CASE("esm_reader_t::set_key, independent of value state", "[i]")
{
	auto tes3_body = make_sub_record("HEDR", std::string(300, '\0'));
	auto tes3 = make_record("TES3", tes3_body);

	auto gmst_body =
	    make_sub_record("NAME", std::string("sSetting\0", 9)) + make_sub_record("STRV", std::string("Value\0", 6));
	auto gmst = make_record("GMST", gmst_body);

	std::string file_content = tes3 + gmst;

	const auto temp_path = get_temp_path("yampt_test_key_indep.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	REQUIRE(reader.is_loaded());

	reader.select_record(1);
	reader.set_value("STRV");
	REQUIRE(reader.get_value().exist == true);
	REQUIRE(reader.get_value().text == "Value");

	reader.set_key("NAME");
	REQUIRE(reader.get_key().exist == true);
	REQUIRE(reader.get_key().text == "sSetting");
	REQUIRE(reader.get_value().exist == true);
	REQUIRE(reader.get_value().text == "Value");
}

TEST_CASE("esm_reader_t::set_value, resets counter to zero", "[i]")
{
	auto tes3_body = make_sub_record("HEDR", std::string(300, '\0'));
	auto tes3 = make_record("TES3", tes3_body);

	auto rnam1 = std::string("First\0", 6);
	auto rnam2 = std::string("Second\0", 7);

	auto fact_body = make_sub_record("NAME", std::string("Guild\0", 6)) + make_sub_record("RNAM", rnam1) +
	                 make_sub_record("RNAM", rnam2);
	auto fact = make_record("FACT", fact_body);

	std::string file_content = tes3 + fact;

	const auto temp_path = get_temp_path("yampt_test_value_reset.esm");
	binary_file_io::write_text(file_content, temp_path);
	esm_reader_t reader(temp_path);
	std::filesystem::remove(temp_path);

	reader.select_record(1);
	reader.set_value("RNAM");
	reader.set_next_value("RNAM");
	REQUIRE(reader.get_value().counter == 1);

	reader.set_value("RNAM");
	REQUIRE(reader.get_value().counter == 0);
	REQUIRE(reader.get_value().text == "First");
}
