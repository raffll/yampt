#include <catch2/catch_all.hpp>
#include <io/yaml_scalar.hpp>

TEST_CASE("yaml_scalar_t::decode_quoted, plain content", "[u]")
{
	REQUIRE(yaml_scalar_t::decode_quoted("\"hello world\"") == "hello world");
}

TEST_CASE("yaml_scalar_t::decode_quoted, newline escape", "[u]")
{
	REQUIRE(yaml_scalar_t::decode_quoted("\"first\\nsecond\"") == "first\nsecond");
}

TEST_CASE("yaml_scalar_t::decode_quoted, tab escape", "[u]")
{
	REQUIRE(yaml_scalar_t::decode_quoted("\"a\\tb\"") == "a\tb");
}

TEST_CASE("yaml_scalar_t::decode_quoted, carriage return escape", "[u]")
{
	REQUIRE(yaml_scalar_t::decode_quoted("\"a\\rb\"") == "a\rb");
}

TEST_CASE("yaml_scalar_t::decode_quoted, quote escape", "[u]")
{
	REQUIRE(yaml_scalar_t::decode_quoted("\"say \\\"hi\\\"\"") == "say \"hi\"");
}

TEST_CASE("yaml_scalar_t::decode_quoted, backslash escape", "[u]")
{
	REQUIRE(yaml_scalar_t::decode_quoted("\"a\\\\b\"") == "a\\b");
}

TEST_CASE("yaml_scalar_t::decode_quoted, null escape", "[u]")
{
	REQUIRE(yaml_scalar_t::decode_quoted("\"a\\0b\"") == std::string("a\0b", 3));
}

TEST_CASE("yaml_scalar_t::decode_quoted, unknown escape passes through", "[u]")
{
	REQUIRE(yaml_scalar_t::decode_quoted("\"a\\zb\"") == "a\\zb");
}

TEST_CASE("yaml_scalar_t::decode_quoted, stops at closing quote", "[u]")
{
	REQUIRE(yaml_scalar_t::decode_quoted("\"visible\" trailing") == "visible");
}

TEST_CASE("yaml_scalar_t::decode_quoted, empty quoted", "[u]")
{
	REQUIRE(yaml_scalar_t::decode_quoted("\"\"").empty());
}

TEST_CASE("yaml_scalar_t::parse_block_indicator, recognizes all three", "[u]")
{
	REQUIRE(yaml_scalar_t::parse_block_indicator("|").value() == yaml_scalar_t::chomp_t::clip);
	REQUIRE(yaml_scalar_t::parse_block_indicator("|-").value() == yaml_scalar_t::chomp_t::strip);
	REQUIRE(yaml_scalar_t::parse_block_indicator("|+").value() == yaml_scalar_t::chomp_t::keep);
}

TEST_CASE("yaml_scalar_t::parse_block_indicator, rejects non-block", "[u]")
{
	REQUIRE_FALSE(yaml_scalar_t::parse_block_indicator("plain value").has_value());
	REQUIRE_FALSE(yaml_scalar_t::parse_block_indicator(">").has_value());
	REQUIRE_FALSE(yaml_scalar_t::parse_block_indicator("").has_value());
}

TEST_CASE("yaml_scalar_t::apply_chomp, clip keeps body unchanged", "[u]")
{
	REQUIRE(yaml_scalar_t::apply_chomp("a\nb", yaml_scalar_t::chomp_t::clip) == "a\nb");
}

TEST_CASE("yaml_scalar_t::apply_chomp, strip removes trailing newlines", "[u]")
{
	REQUIRE(yaml_scalar_t::apply_chomp("a\nb\n\n", yaml_scalar_t::chomp_t::strip) == "a\nb");
	REQUIRE(yaml_scalar_t::apply_chomp("a\nb", yaml_scalar_t::chomp_t::strip) == "a\nb");
}

TEST_CASE("yaml_scalar_t::apply_chomp, keep ensures trailing newline", "[u]")
{
	REQUIRE(yaml_scalar_t::apply_chomp("a\nb", yaml_scalar_t::chomp_t::keep) == "a\nb\n");
	REQUIRE(yaml_scalar_t::apply_chomp("a\nb\n", yaml_scalar_t::chomp_t::keep) == "a\nb\n");
}
