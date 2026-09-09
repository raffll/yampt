#include <catch2/catch_all.hpp>
#include <scanner/exclusion_resolver.hpp>

TEST_CASE("exclusion_resolver_t::is_record_excluded, anchored id matches", "[u]")
{
	exclusion_resolver_t resolver;
	resolver.set_pattern("^" + exclusion_resolver::regex_escape_literal("BM_bear_black") + "$");

	REQUIRE(resolver.is_record_excluded("CREA", "BM_bear_black"));
}

TEST_CASE("exclusion_resolver_t::is_record_excluded, unrelated id does not match", "[u]")
{
	exclusion_resolver_t resolver;
	resolver.set_pattern("^" + exclusion_resolver::regex_escape_literal("BM_bear_black") + "$");

	REQUIRE_FALSE(resolver.is_record_excluded("CREA", "BM_bear_brown"));
}

TEST_CASE("exclusion_resolver_t::is_record_excluded, disabled type is excluded", "[u]")
{
	exclusion_resolver_t resolver;
	resolver.set_disabled_types({ "CELL" });

	REQUIRE(resolver.is_record_excluded("CELL", "Balmora"));
	REQUIRE_FALSE(resolver.is_record_excluded("CREA", "Balmora"));
}

TEST_CASE("exclusion_resolver_t::is_record_excluded, empty pattern excludes nothing", "[u]")
{
	exclusion_resolver_t resolver;
	resolver.set_pattern("");

	REQUIRE_FALSE(resolver.is_record_excluded("CREA", "anything"));
}

TEST_CASE("exclusion_resolver_t::is_record_excluded, invalid pattern excludes nothing", "[u]")
{
	exclusion_resolver_t resolver;
	resolver.set_pattern("[unterminated");

	REQUIRE_FALSE(resolver.is_record_excluded("CREA", "anything"));
}

TEST_CASE("exclusion_resolver_t::is_record_excluded, case-insensitive match", "[u]")
{
	exclusion_resolver_t resolver;
	resolver.set_pattern("^balmora$");

	REQUIRE(resolver.is_record_excluded("CELL", "BALMORA"));
}

TEST_CASE("exclusion_resolver_t::is_record_excluded, regex_search substring match", "[u]")
{
	exclusion_resolver_t resolver;
	resolver.set_pattern("bear");

	REQUIRE(resolver.is_record_excluded("CREA", "BM_bear_black"));
}

TEST_CASE("exclusion_resolver::regex_escape_literal, escapes metacharacters", "[u]")
{
	const auto escaped = exclusion_resolver::regex_escape_literal("a.b+c(d)[e]{f}^g$h|i?j*k\\l");

	REQUIRE(escaped == "a\\.b\\+c\\(d\\)\\[e\\]\\{f\\}\\^g\\$h\\|i\\?j\\*k\\\\l");
}

TEST_CASE("exclusion_resolver::regex_escape_literal, anchored id with metacharacters matches literally", "[u]")
{
	const auto id = "item(special).v2";

	exclusion_resolver_t resolver;
	resolver.set_pattern("^" + exclusion_resolver::regex_escape_literal(id) + "$");

	REQUIRE(resolver.is_record_excluded("MISC", id));
	REQUIRE_FALSE(resolver.is_record_excluded("MISC", "itemXspecialXvY2"));
}
