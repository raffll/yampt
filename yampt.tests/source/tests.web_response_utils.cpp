#include <catch2/catch_all.hpp>
#include <translator/web_response_utils.hpp>
#include <QJsonDocument>

TEST_CASE("web_response_utils::json_value_from_string, integer becomes number", "[u]")
{
	const auto value = web_response_utils::json_value_from_string("42");
	REQUIRE(value.isDouble());
	REQUIRE(value.toInt() == 42);
}

TEST_CASE("web_response_utils::json_value_from_string, float becomes number", "[u]")
{
	const auto value = web_response_utils::json_value_from_string("0.7");
	REQUIRE(value.isDouble());
	REQUIRE(value.toDouble() == Catch::Approx(0.7));
}

TEST_CASE("web_response_utils::json_value_from_string, negative float becomes number", "[u]")
{
	const auto value = web_response_utils::json_value_from_string("-1.5");
	REQUIRE(value.isDouble());
	REQUIRE(value.toDouble() == Catch::Approx(-1.5));
}

TEST_CASE("web_response_utils::json_value_from_string, non-numeric stays string", "[u]")
{
	const auto value = web_response_utils::json_value_from_string("claude-sonnet-4");
	REQUIRE(value.isString());
	REQUIRE(value.toString().toStdString() == "claude-sonnet-4");
}

TEST_CASE("web_response_utils::json_value_from_string, empty stays string", "[u]")
{
	const auto value = web_response_utils::json_value_from_string("");
	REQUIRE(value.isString());
	REQUIRE(value.toString().isEmpty());
}

static QJsonDocument parse(const std::string & json)
{
	return QJsonDocument::fromJson(QByteArray::fromStdString(json));
}

TEST_CASE("web_response_utils::extract_by_path, nested object field", "[u]")
{
	const auto doc = parse(R"({"data":{"text":"hello"}})");
	REQUIRE(web_response_utils::extract_by_path(doc, "data.text") == "hello");
}

TEST_CASE("web_response_utils::extract_by_path, array index", "[u]")
{
	const auto doc = parse(R"({"translations":[{"text":"bonjour"}]})");
	REQUIRE(web_response_utils::extract_by_path(doc, "translations[0].text") == "bonjour");
}

TEST_CASE("web_response_utils::extract_by_path, anthropic content array", "[u]")
{
	const auto doc = parse(R"({"content":[{"text":"witaj"}]})");
	REQUIRE(web_response_utils::extract_by_path(doc, "content[0].text") == "witaj");
}

TEST_CASE("web_response_utils::extract_by_path, missing closing bracket returns empty", "[u]")
{
	const auto doc = parse(R"({"translations":[{"text":"bonjour"}]})");
	REQUIRE(web_response_utils::extract_by_path(doc, "translations[0.text").empty());
}

TEST_CASE("web_response_utils::extract_by_path, non-numeric index returns empty", "[u]")
{
	const auto doc = parse(R"({"translations":[{"text":"bonjour"}]})");
	REQUIRE(web_response_utils::extract_by_path(doc, "translations[x].text").empty());
}

TEST_CASE("web_response_utils::extract_by_path, out of range index returns empty", "[u]")
{
	const auto doc = parse(R"({"translations":[{"text":"bonjour"}]})");
	REQUIRE(web_response_utils::extract_by_path(doc, "translations[5].text").empty());
}

TEST_CASE("web_response_utils::extract_by_path, missing field returns empty", "[u]")
{
	const auto doc = parse(R"({"data":{"text":"hello"}})");
	REQUIRE(web_response_utils::extract_by_path(doc, "data.missing").empty());
}

TEST_CASE("web_response_utils::extract_by_path, non-string leaf returns empty", "[u]")
{
	const auto doc = parse(R"({"data":{"count":5}})");
	REQUIRE(web_response_utils::extract_by_path(doc, "data.count").empty());
}

TEST_CASE("web_response_utils::extract_by_path, invalid json returns empty", "[u]")
{
	const auto doc = parse("not json");
	REQUIRE(web_response_utils::extract_by_path(doc, "data.text").empty());
}
