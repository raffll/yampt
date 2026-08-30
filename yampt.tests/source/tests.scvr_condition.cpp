#include <catch2/catch_all.hpp>
#include <decoder/scvr_condition.hpp>
#include <cstring>

TEST_CASE("scvr_condition::parse_scvr_condition, journal condition", "[u]")
{
	const char data[] = "04JX3B8_MeetVivec";
	auto condition = parse_scvr_condition(data, sizeof(data) - 1);
	REQUIRE(condition.valid);
	REQUIRE(condition.index == 0);
	REQUIRE(condition.type_name == "Journal");
	REQUIRE(condition.operator_symbol == ">=");
	REQUIRE(condition.variable_name == "B8_MeetVivec");
}

TEST_CASE("scvr_condition::parse_scvr_condition, dead condition", "[u]")
{
	const char data[] = "06DX2arius rulician";
	auto condition = parse_scvr_condition(data, sizeof(data) - 1);
	REQUIRE(condition.valid);
	REQUIRE(condition.index == 0);
	REQUIRE(condition.type_name == "Dead");
	REQUIRE(condition.operator_symbol == ">");
	REQUIRE(condition.variable_name == "arius rulician");
}

TEST_CASE("scvr_condition::parse_scvr_condition, not local condition", "[u]")
{
	const char data[] = "0CsX0nolore";
	auto condition = parse_scvr_condition(data, sizeof(data) - 1);
	REQUIRE(condition.valid);
	REQUIRE(condition.type_name == "Not Local");
	REQUIRE(condition.operator_symbol == "==");
	REQUIRE(condition.variable_name == "nolore");
}

TEST_CASE("scvr_condition::parse_scvr_condition, item condition", "[u]")
{
	const char data[] = "25IX2bk_talostreason";
	auto condition = parse_scvr_condition(data, sizeof(data) - 1);
	REQUIRE(condition.valid);
	REQUIRE(condition.index == 2);
	REQUIRE(condition.type_name == "Item");
	REQUIRE(condition.operator_symbol == ">");
	REQUIRE(condition.variable_name == "bk_talostreason");
}

TEST_CASE("scvr_condition::parse_scvr_condition, function choice resolves name", "[u]")
{
	const char buffer[5] = { '0', '1', '5', '0', '0' };
	auto condition = parse_scvr_condition(buffer, 5);
	REQUIRE(condition.valid);
	REQUIRE(condition.type_name == "Function");
	REQUIRE(condition.function_name == "Choice");
	REQUIRE(condition.operator_symbol == "==");
}

TEST_CASE("scvr_condition::parse_scvr_condition, too short is invalid", "[u]")
{
	const char data[] = "04J";
	auto condition = parse_scvr_condition(data, sizeof(data) - 1);
	REQUIRE_FALSE(condition.valid);
}

TEST_CASE("scvr_condition::parse_scvr_condition, invalid type char is invalid", "[u]")
{
	const char data[] = "0ZX0name";
	auto condition = parse_scvr_condition(data, sizeof(data) - 1);
	REQUIRE_FALSE(condition.valid);
}

TEST_CASE("scvr_condition::scvr_operator_symbol, all operators", "[u]")
{
	REQUIRE(std::strcmp(scvr_operator_symbol('0'), "==") == 0);
	REQUIRE(std::strcmp(scvr_operator_symbol('1'), "!=") == 0);
	REQUIRE(std::strcmp(scvr_operator_symbol('2'), ">") == 0);
	REQUIRE(std::strcmp(scvr_operator_symbol('3'), ">=") == 0);
	REQUIRE(std::strcmp(scvr_operator_symbol('4'), "<") == 0);
	REQUIRE(std::strcmp(scvr_operator_symbol('5'), "<=") == 0);
}

TEST_CASE("scvr_condition::format_scvr_condition, journal with value", "[u]")
{
	const char data[] = "34JX3IL_TalosTreason";
	auto condition = parse_scvr_condition(data, sizeof(data) - 1);
	auto result = format_scvr_condition(condition, "100");
	REQUIRE(result == "Journal \"IL_TalosTreason\" >= 100");
}

TEST_CASE("scvr_condition::format_scvr_condition, invalid condition is empty", "[u]")
{
	scvr_condition_t condition;
	auto result = format_scvr_condition(condition, "1");
	REQUIRE(result.empty());
}

TEST_CASE("scvr_condition::scvr_subject_display, function resolves name", "[u]")
{
	const char buffer[5] = { '0', '1', '5', '0', '0' };
	REQUIRE(scvr_subject_display(buffer, 5) == "Choice");
	REQUIRE(scvr_subject_label(buffer, 5) == "Function");
}

TEST_CASE("scvr_condition::scvr_subject_display, local shows storage type", "[u]")
{
	const char data[] = "03sX0myvar";
	REQUIRE(scvr_subject_display(data, sizeof(data) - 1) == "Short");
	REQUIRE(scvr_subject_label(data, sizeof(data) - 1) == "Variable Type");
}

TEST_CASE("scvr_condition::scvr_subject_display, journal shows marker", "[u]")
{
	const char data[] = "04JX3B8_MeetVivec";
	REQUIRE(scvr_subject_display(data, sizeof(data) - 1) == "JX");
	REQUIRE(scvr_subject_label(data, sizeof(data) - 1) == "Marker");
}

TEST_CASE("scvr_condition::scvr_type_name, known and unknown chars", "[u]")
{
	REQUIRE(scvr_type_name('4') == "Journal");
	REQUIRE(scvr_type_name('6') == "Dead");
	REQUIRE(scvr_type_name('C') == "Not Local");
	REQUIRE(scvr_type_name('Z').empty());
}

TEST_CASE("scvr_condition::scvr_type_char, round trips names", "[u]")
{
	REQUIRE(scvr_type_char("Journal") == '4');
	REQUIRE(scvr_type_char("Dead") == '6');
	REQUIRE(scvr_type_char("Not Local") == 'C');
	REQUIRE(scvr_type_char("Nonsense") == '\0');
}

TEST_CASE("scvr_condition::scvr_operator_char, round trips symbols", "[u]")
{
	REQUIRE(scvr_operator_char("==") == '0');
	REQUIRE(scvr_operator_char("!=") == '1');
	REQUIRE(scvr_operator_char(">=") == '3');
	REQUIRE(scvr_operator_char("<=") == '5');
	REQUIRE(scvr_operator_char("??") == '\0');
}

TEST_CASE("scvr_condition::scvr_type_names, matches symbol count", "[u]")
{
	REQUIRE(scvr_type_names().size() == 12);
	REQUIRE(scvr_operator_symbols().size() == 6);
}

TEST_CASE("scvr_condition::scvr_variable_storage_name, known chars", "[u]")
{
	REQUIRE(std::strcmp(scvr_variable_storage_name('f'), "Float") == 0);
	REQUIRE(std::strcmp(scvr_variable_storage_name('l'), "Long") == 0);
	REQUIRE(std::strcmp(scvr_variable_storage_name('s'), "Short") == 0);
	REQUIRE(scvr_variable_storage_name('X')[0] == '\0');
}

TEST_CASE("scvr_condition::scvr_type_uses_function, only function", "[u]")
{
	REQUIRE(scvr_type_uses_function("Function"));
	REQUIRE_FALSE(scvr_type_uses_function("Journal"));
	REQUIRE_FALSE(scvr_type_uses_function("Local"));
}

TEST_CASE("scvr_condition::scvr_type_uses_variable_storage, global local not local", "[u]")
{
	REQUIRE(scvr_type_uses_variable_storage("Global"));
	REQUIRE(scvr_type_uses_variable_storage("Local"));
	REQUIRE(scvr_type_uses_variable_storage("Not Local"));
	REQUIRE_FALSE(scvr_type_uses_variable_storage("Journal"));
	REQUIRE_FALSE(scvr_type_uses_variable_storage("Function"));
}
