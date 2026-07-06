#include <catch2/catch_all.hpp>
#include <converter/esm_converter.hpp>
#include <io/dict_writer.hpp>
#include <utility/app_logger.hpp>
#include <utility/includes.hpp>

static std::string get_temp_path(const std::string & filename)
{
	return (std::filesystem::temp_directory_path() / filename).string();
}

static std::string make_sub_record(const std::string & sub_id, const std::string & content)
{
	std::string result;
	result += sub_id;
	result += domain_types::convert_uint_to_string_byte_array(content.size());
	result += content;
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

static std::string make_tes3_record()
{
	return make_record("TES3", make_sub_record("HEDR", std::string(300, '\0')));
}

static dict_merger_t make_merger(
    const std::vector<std::tuple<rec_type_t, std::string, std::string, std::string>> & entries)
{
	const auto dict_path = get_temp_path("yampt_test_counters_dict.json");
	app_logger_t::reset_log();

	dict_t dict = domain_types::initialize_dict();
	for (const auto & [record_type, key_text, old_text, new_text] : entries)
		dict.at(record_type).insert({ key_text, old_text, new_text, status_t::translated });

	dict_writer_t::write(dict, dict_path);

	app_logger_t::reset_log();
	dict_merger_t merger({ dict_path });
	app_logger_t::reset_log();
	std::filesystem::remove(dict_path);
	return merger;
}

static void write_esm_file(const std::string & content, const std::string & path)
{
	std::ofstream file(path, std::ios::binary);
	file.write(content.data(), content.size());
}

struct counter_values_t
{
	int converted = -1;
	int identical = -1;
	int unchanged = -1;
	int total = -1;
};

static counter_values_t parse_log_counters(const std::string & log, const std::string & prefix)
{
	counter_values_t result;
	auto line_pos = log.find(prefix + ":");
	if (line_pos == std::string::npos)
		return result;

	auto line_end = log.find("\r\n", line_pos);
	auto line = log.substr(line_pos, line_end - line_pos);

	auto parse_field = [&](const std::string & field_name) -> int
	{
		auto field_pos = line.find(field_name + "=");
		if (field_pos == std::string::npos)
			return -1;

		auto value_start = field_pos + field_name.size() + 1;
		auto value_end = line.find_first_of(",\r\n", value_start);
		return std::stoi(line.substr(value_start, value_end - value_start));
	};

	result.converted = parse_field("converted");
	result.identical = parse_field("identical");
	result.unchanged = parse_field("unchanged");
	result.total = parse_field("total");
	return result;
}

TEST_CASE("esm_converter_t::convert_scpt, SCTX counters sum correctly", "[i]")
{
	auto merger = make_merger(
	    {
	        { rec_type_t::cell, "Balmora", "Balmora", "Balmora PL" },
	    });

	std::string old_sctx_changed = "GetPCCell \"Balmora\"";
	std::string old_sctx_identical = "set x to 1";

	auto make_scdt = [](const std::string & text_in_scdt)
	{
		std::string scdt;
		scdt += std::string(10, '\x00');
		scdt += std::string(1, static_cast<char>(text_in_scdt.size()));
		scdt += text_in_scdt;
		scdt += std::string(5, '\x00');
		return scdt;
	};

	auto make_schd = [](const std::string & name, size_t scdt_size)
	{
		std::string schd(48, '\0');
		schd.replace(0, name.size(), name);
		schd.erase(44, 4);
		schd.insert(44, domain_types::convert_uint_to_string_byte_array(scdt_size));
		return schd;
	};

	auto scdt_changed = make_scdt("Balmora");
	auto scdt_identical = make_scdt("something");

	auto scpt_changed = make_record(
	    "SCPT",
	    make_sub_record("SCHD", make_schd("ScriptA", scdt_changed.size())) + make_sub_record("SCDT", scdt_changed) +
	        make_sub_record("SCTX", old_sctx_changed));

	auto scpt_identical = make_record(
	    "SCPT",
	    make_sub_record("SCHD", make_schd("ScriptB", scdt_identical.size())) + make_sub_record("SCDT", scdt_identical) +
	        make_sub_record("SCTX", old_sctx_identical));

	auto esm_content = make_tes3_record() + scpt_changed + scpt_identical;

	const auto esm_path = get_temp_path("yampt_test_sctx_counters.esm");
	write_esm_file(esm_content, esm_path);
	app_logger_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & log = app_logger_t::get_log();
	auto counters = parse_log_counters(log, "SCTX");

	REQUIRE(counters.total == 2);
	REQUIRE(counters.converted == 1);
	REQUIRE(counters.identical == 1);
	REQUIRE(counters.unchanged == 0);
	REQUIRE(counters.converted + counters.identical + counters.unchanged == counters.total);
}

TEST_CASE("esm_converter_t::convert_bnam, BNAM counters sum correctly", "[i]")
{
	std::string topic_type(1, '\0');
	auto dial_record =
	    make_record("DIAL", make_sub_record("NAME", std::string("topic\0", 6)) + make_sub_record("DATA", topic_type));

	auto info_changed = make_record(
	    "INFO",
	    make_sub_record("INAM", std::string("info_01\0", 8)) +
	        make_sub_record("BNAM", std::string("AddTopic \"Hello\"", 16)));

	auto info_identical = make_record(
	    "INFO",
	    make_sub_record("INAM", std::string("info_02\0", 8)) + make_sub_record("BNAM", std::string("set x to 1", 10)));

	auto info_no_bnam = make_record(
	    "INFO", make_sub_record("INAM", std::string("info_03\0", 8)) + make_sub_record("NAME", std::string("text", 4)));

	auto esm_content = make_tes3_record() + dial_record + info_changed + info_identical + info_no_bnam;

	const auto dict_path = get_temp_path("yampt_test_bnam_counters_dict.json");
	app_logger_t::reset_log();

	dict_t dict = domain_types::initialize_dict();
	dict.at(rec_type_t::dial).insert({ "Hello", "Hello", "Witaj", status_t::translated });
	dict_writer_t::write(dict, dict_path);

	app_logger_t::reset_log();
	dict_merger_t merger({ dict_path });
	app_logger_t::reset_log();
	std::filesystem::remove(dict_path);

	const auto esm_path = get_temp_path("yampt_test_bnam_counters.esm");
	write_esm_file(esm_content, esm_path);
	app_logger_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & log = app_logger_t::get_log();
	auto counters = parse_log_counters(log, "BNAM");

	REQUIRE(counters.total == 2);
	REQUIRE(counters.converted == 1);
	REQUIRE(counters.identical == 1);
	REQUIRE(counters.converted + counters.identical + counters.unchanged == counters.total);
}

TEST_CASE("esm_converter_t::convert_cell, CELL counters sum correctly", "[i]")
{
	auto merger = make_merger(
	    {
	        { rec_type_t::cell, "Balmora", "Balmora", "Balmora PL" },
	    });

	auto cell_changed = make_record("CELL", make_sub_record("NAME", std::string("Balmora\0", 8)));
	auto cell_identical = make_record("CELL", make_sub_record("NAME", std::string("Unknown\0", 8)));

	auto esm_content = make_tes3_record() + cell_changed + cell_identical;

	const auto esm_path = get_temp_path("yampt_test_cell_counters.esm");
	write_esm_file(esm_content, esm_path);
	app_logger_t::reset_log();

	esm_converter_t converter(esm_path, merger, false, "", codepage_t::windows_1252, false);
	std::filesystem::remove(esm_path);

	REQUIRE(converter.is_loaded());

	const auto & log = app_logger_t::get_log();
	auto counters = parse_log_counters(log, "CELL");

	REQUIRE(counters.total == 2);
	REQUIRE(counters.converted == 1);
	REQUIRE(counters.unchanged == 1);
	REQUIRE(counters.converted + counters.identical + counters.unchanged == counters.total);
}
