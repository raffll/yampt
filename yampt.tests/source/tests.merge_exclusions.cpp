#include <catch2/catch_all.hpp>
#include <scanner/merge_exclusions.hpp>

namespace {
merge_exclusions_t make_with(const std::vector<exclude_rule_t> & rules)
{
	merge_exclusions_t exclusions;
	exclusions.set_rules(rules);
	return exclusions;
}
} // namespace

TEST_CASE("merge_exclusions_t::is_record_excluded, anchored id matches", "[u]")
{
	const auto exclusions =
	    make_with({ { exclude_kind_t::record_id, merge_exclusions::anchored_id_token("BM_bear_black") } });

	REQUIRE(exclusions.is_record_excluded("CREA", "BM_bear_black"));
}

TEST_CASE("merge_exclusions_t::is_record_excluded, unrelated id does not match", "[u]")
{
	const auto exclusions =
	    make_with({ { exclude_kind_t::record_id, merge_exclusions::anchored_id_token("BM_bear_black") } });

	REQUIRE_FALSE(exclusions.is_record_excluded("CREA", "BM_bear_brown"));
}

TEST_CASE("merge_exclusions_t::is_record_excluded, excluded type is excluded", "[u]")
{
	const auto exclusions = make_with({ { exclude_kind_t::record_type, "CELL" } });

	REQUIRE(exclusions.is_record_excluded("CELL", "Balmora"));
	REQUIRE(exclusions.is_type_excluded("CELL"));
	REQUIRE_FALSE(exclusions.is_type_excluded("CREA"));
}

TEST_CASE("merge_exclusions_t::is_record_excluded, no rules excludes nothing", "[u]")
{
	const auto exclusions = make_with({});

	REQUIRE_FALSE(exclusions.is_record_excluded("CREA", "anything"));
}

TEST_CASE("merge_exclusions_t::is_record_excluded, invalid regex excludes nothing", "[u]")
{
	const auto exclusions = make_with({ { exclude_kind_t::record_id, "[unterminated" } });

	REQUIRE_FALSE(exclusions.is_record_excluded("CREA", "anything"));
}

TEST_CASE("merge_exclusions_t::is_record_excluded, case-insensitive match", "[u]")
{
	const auto exclusions = make_with({ { exclude_kind_t::record_id, "^balmora$" } });

	REQUIRE(exclusions.is_record_excluded("CELL", "Balmora"));
}

TEST_CASE("merge_exclusions_t::is_record_excluded, substring regex match", "[u]")
{
	const auto exclusions = make_with({ { exclude_kind_t::record_id, "bear" } });

	REQUIRE(exclusions.is_record_excluded("CREA", "BM_bear_black"));
}

TEST_CASE("merge_exclusions_t::is_file_excluded, case-insensitive filename match", "[u]")
{
	const auto exclusions = make_with({ { exclude_kind_t::file, "SomeMod.esp" } });

	REQUIRE(exclusions.is_file_excluded("somemod.esp"));
	REQUIRE_FALSE(exclusions.is_file_excluded("Other.esp"));
}

TEST_CASE("merge_exclusions_t::ignored_sub_records, collects only sub-record rules", "[u]")
{
	const auto exclusions = make_with(
	    { { exclude_kind_t::sub_record, "CELL:NAM0" },
	      { exclude_kind_t::record_type, "REGN" },
	      { exclude_kind_t::sub_record, "LTEX:INTV" } });

	const auto subs = exclusions.ignored_sub_records();
	REQUIRE(subs.size() == 2);
	REQUIRE(subs.count("CELL:NAM0") == 1);
	REQUIRE(subs.count("LTEX:INTV") == 1);
}

TEST_CASE("merge_exclusions::regex_escape_literal, escapes metacharacters", "[u]")
{
	const auto escaped = merge_exclusions::regex_escape_literal("a.b+c(d)[e]{f}^g$h|i?j*k\\l");

	REQUIRE(escaped == "a\\.b\\+c\\(d\\)\\[e\\]\\{f\\}\\^g\\$h\\|i\\?j\\*k\\\\l");
}

TEST_CASE("merge_exclusions_t::is_record_excluded, anchored id with metacharacters matches literally", "[u]")
{
	const std::string id = "item(special).v2";
	const auto exclusions = make_with({ { exclude_kind_t::record_id, merge_exclusions::anchored_id_token(id) } });

	REQUIRE(exclusions.is_record_excluded("MISC", id));
}

TEST_CASE("merge_exclusions_t::parse, round-trips kinds and targets", "[u]")
{
	const std::vector<exclude_rule_t> rules {
		{ exclude_kind_t::file, "SomeMod.esp" },
		{ exclude_kind_t::record_id, "^MyMod_.*" },
		{ exclude_kind_t::record_type, "REGN" },
		{ exclude_kind_t::sub_record, "CELL:NAM0" }
	};

	const auto serialized = merge_exclusions_t::serialize(rules);
	const auto parsed = merge_exclusions_t::parse(serialized);

	REQUIRE(parsed == rules);
}

TEST_CASE("merge_exclusions_t::parse, skips blank and malformed lines", "[u]")
{
	const auto parsed = merge_exclusions_t::parse("file:A.esp\n\n  \nnokindhere\nid:^x$\n");

	REQUIRE(parsed.size() == 2);
	REQUIRE(parsed[0] == exclude_rule_t { exclude_kind_t::file, "A.esp" });
	REQUIRE(parsed[1] == exclude_rule_t { exclude_kind_t::record_id, "^x$" });
}
