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

TEST_CASE("scvr_condition::format_scvr_condition, function with value", "[u]")
{
	const char buffer[5] = { '0', '1', '5', '0', '0' };
	auto condition = parse_scvr_condition(buffer, 5);
	auto result = format_scvr_condition(condition, "5");
	REQUIRE(result == "Function Choice == 5");
}

TEST_CASE("scvr_condition::format_scvr_condition, invalid condition is empty", "[u]")
{
	scvr_condition_t condition;
	auto result = format_scvr_condition(condition, "1");
	REQUIRE(result.empty());
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
