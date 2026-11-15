#include <catch2/catch_all.hpp>
#include <creator/info_place_values.hpp>
#include <utility/domain_types.hpp>

namespace {

std::string build_sub_record(const std::string & sub_id, const std::string & content)
{
	return sub_id + domain_types::convert_uint_to_string_byte_array(content.size()) + content;
}

std::string build_record(const std::string & record_type, const std::string & body)
{
	std::string header = record_type;
	header += domain_types::convert_uint_to_string_byte_array(body.size());
	header += domain_types::convert_uint_to_string_byte_array(0);
	header += domain_types::convert_uint_to_string_byte_array(0);
	return header + body;
}

std::string build_scvr_condition(char type_char, const std::string & value_text)
{
	std::string condition = "0";
	condition += type_char;
	condition += "00";
	condition += '0';
	return condition + value_text;
}

} // namespace

TEST_CASE("info_place_values::collect_from_info_record, ANAM place is collected without terminator", "[eet_import]")
{
	const auto record = build_record("INFO", build_sub_record("ANAM", std::string("Balmora") + '\0'));

	std::set<std::string> places;
	info_place_values::collect_from_info_record(record, places);

	REQUIRE(places == std::set<std::string> { "Balmora" });
}

TEST_CASE("info_place_values::collect_from_info_record, empty ANAM is ignored", "[eet_import]")
{
	const auto record = build_record("INFO", build_sub_record("ANAM", std::string(1, '\0')));

	std::set<std::string> places;
	info_place_values::collect_from_info_record(record, places);

	REQUIRE(places.empty());
}

TEST_CASE("info_place_values::collect_from_info_record, SCVR cell condition value is collected", "[eet_import]")
{
	std::string body;
	body += build_sub_record("SCVR", build_scvr_condition('B', "Vivec, Temple"));
	body += build_sub_record("SCVR", build_scvr_condition('B', "Ald-ruhn"));

	std::set<std::string> places;
	info_place_values::collect_from_info_record(build_record("INFO", body), places);

	REQUIRE(places == std::set<std::string> { "Vivec, Temple", "Ald-ruhn" });
}

TEST_CASE("info_place_values::collect_from_info_record, non-cell SCVR condition is ignored", "[eet_import]")
{
	const auto record = build_record("INFO", build_sub_record("SCVR", build_scvr_condition('1', "fargoth")));

	std::set<std::string> places;
	info_place_values::collect_from_info_record(record, places);

	REQUIRE(places.empty());
}

TEST_CASE("info_place_values::collect_from_info_record, SCVR without value text is ignored", "[eet_import]")
{
	const auto record = build_record("INFO", build_sub_record("SCVR", build_scvr_condition('B', "")));

	std::set<std::string> places;
	info_place_values::collect_from_info_record(record, places);

	REQUIRE(places.empty());
}
