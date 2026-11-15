#include <catch2/catch_all.hpp>
#include <scanner/tree_source.hpp>

namespace {

conflict_entry_t make_entry(const std::vector<record_version_t> & versions)
{
	conflict_entry_t entry;
	entry.rec_type = "WEAP";
	entry.record_id = "test_weapon";
	entry.versions = versions;

	return entry;
}

record_version_t make_version(int plugin_idx, conflict_this_t status)
{
	record_version_t version;
	version.plugin_idx = plugin_idx;
	version.record_index = 0;
	version.status = status;

	return version;
}

} // namespace

TEST_CASE("tree_source_t::icon_flags, guard overrides every other tier", "[u]")
{
	plugin_meta_t meta;
	meta.filename = "Merged Patch.esp";
	meta.path = "C:/mods/overwrite/Merged Patch.esp";
	meta.is_guard = true;
	meta.is_active = true;

	const auto flags = tree_source_t::icon_flags(meta);

	REQUIRE(flags.is_guard);
	REQUIRE(flags.is_overridden);
	REQUIRE(flags.is_active);
	REQUIRE(flags.filename == "Merged Patch.esp");
}

TEST_CASE("tree_source_t::icon_flags, overwrite path sets is_overridden", "[u]")
{
	plugin_meta_t meta;
	meta.filename = "plugin.esp";
	meta.path = "C:\\mods\\overwrite\\plugin.esp";

	const auto flags = tree_source_t::icon_flags(meta);

	REQUIRE(flags.is_overridden);
	REQUIRE_FALSE(flags.is_guard);
	REQUIRE_FALSE(flags.is_active);
}

TEST_CASE("tree_source_t::icon_flags, plain path leaves overridden unset", "[u]")
{
	plugin_meta_t meta;
	meta.filename = "Morrowind.esm";
	meta.path = "C:/games/data/Morrowind.esm";

	const auto flags = tree_source_t::icon_flags(meta);

	REQUIRE_FALSE(flags.is_overridden);
	REQUIRE_FALSE(flags.is_guard);
	REQUIRE(flags.filename == "Morrowind.esm");
}

TEST_CASE("tree_source_t::record_foreground_class, single version returns unknown", "[u]")
{
	const auto entry = make_entry({ make_version(0, conflict_this_t::conflict_wins) });

	REQUIRE(tree_source_t::record_foreground_class({ entry, 0, false }) == conflict_this_t::unknown);
}

TEST_CASE("tree_source_t::record_foreground_class, returns matching plugin status", "[u]")
{
	const auto entry = make_entry(
	    { make_version(0, conflict_this_t::master), make_version(1, conflict_this_t::conflict_wins) });

	REQUIRE(tree_source_t::record_foreground_class({ entry, 1, false }) == conflict_this_t::conflict_wins);
	REQUIRE(tree_source_t::record_foreground_class({ entry, 0, false }) == conflict_this_t::master);
}

TEST_CASE("tree_source_t::record_foreground_class, absent plugin returns unknown", "[u]")
{
	const auto entry = make_entry(
	    { make_version(0, conflict_this_t::master), make_version(1, conflict_this_t::conflict_wins) });

	REQUIRE(tree_source_t::record_foreground_class({ entry, 7, false }) == conflict_this_t::unknown);
}

TEST_CASE("tree_source_t::record_foreground_class, hide duplicates single plugin returns unknown", "[u]")
{
	const auto entry = make_entry(
	    { make_version(3, conflict_this_t::override_wins), make_version(3, conflict_this_t::conflict_wins) });

	REQUIRE(tree_source_t::record_foreground_class({ entry, 3, true }) == conflict_this_t::unknown);
	REQUIRE(tree_source_t::record_foreground_class({ entry, 3, false }) == conflict_this_t::override_wins);
}

TEST_CASE("tree_source_t::accumulate_worst_this, identical to master does not raise worst", "[u]")
{
	auto worst = conflict_this_t::unknown;
	worst = tree_source_t::accumulate_worst_this(worst, conflict_this_t::identical_to_master);

	REQUIRE(worst == conflict_this_t::unknown);
}

TEST_CASE("tree_source_t::accumulate_worst_this, keeps the higher ranked status", "[u]")
{
	auto worst = conflict_this_t::override_wins;
	worst = tree_source_t::accumulate_worst_this(worst, conflict_this_t::conflict_wins);
	REQUIRE(worst == conflict_this_t::conflict_wins);

	worst = tree_source_t::accumulate_worst_this(worst, conflict_this_t::master);
	REQUIRE(worst == conflict_this_t::conflict_wins);
}

TEST_CASE("tree_source_t::resolve_foreground_class, unknown and master map to identical", "[u]")
{
	REQUIRE(tree_source_t::resolve_foreground_class(conflict_this_t::unknown) == conflict_this_t::identical_to_master);
	REQUIRE(tree_source_t::resolve_foreground_class(conflict_this_t::master) == conflict_this_t::identical_to_master);
}

TEST_CASE("tree_source_t::resolve_foreground_class, other classes pass through", "[u]")
{
	REQUIRE(tree_source_t::resolve_foreground_class(conflict_this_t::conflict_wins) == conflict_this_t::conflict_wins);
	REQUIRE(
	    tree_source_t::resolve_foreground_class(conflict_this_t::identical_to_master) ==
	    conflict_this_t::identical_to_master);
}

TEST_CASE("tree_source_t::unique_plugin_count, counts distinct plugins", "[u]")
{
	const auto entry = make_entry(
	    { make_version(2, conflict_this_t::master),
	      make_version(2, conflict_this_t::conflict_wins),
	      make_version(5, conflict_this_t::override_wins) });

	REQUIRE(tree_source_t::unique_plugin_count(entry) == 2);
}

TEST_CASE("tree_source_t::worst_group_conflict, folds worst this skipping identical to master", "[u]")
{
	std::vector<conflict_entry_t> entries;
	entries.push_back(
	    make_entry({ make_version(0, conflict_this_t::master), make_version(1, conflict_this_t::override_wins) }));
	entries.push_back(
	    make_entry({ make_version(0, conflict_this_t::master), make_version(1, conflict_this_t::identical_to_master) }));
	entries.push_back(
	    make_entry({ make_version(0, conflict_this_t::master), make_version(1, conflict_this_t::conflict_wins) }));

	const std::vector<size_t> entry_indices = { 0, 1, 2 };

	const auto worst = tree_source_t::worst_group_conflict({ entries, entry_indices, 1, false });

	REQUIRE(worst.worst_this == conflict_this_t::conflict_wins);
}

TEST_CASE("tree_source_t::worst_group_conflict, raises worst all to the maximum", "[u]")
{
	auto first = make_entry({ make_version(0, conflict_this_t::master), make_version(1, conflict_this_t::override_wins) });
	first.conflict_all = conflict_all_t::no_conflict;

	auto second = make_entry({ make_version(0, conflict_this_t::master), make_version(1, conflict_this_t::conflict_wins) });
	second.conflict_all = conflict_all_t::conflict;

	auto third = make_entry({ make_version(0, conflict_this_t::master), make_version(1, conflict_this_t::override_wins) });
	third.conflict_all = conflict_all_t::override_benign;

	const std::vector<conflict_entry_t> entries = { first, second, third };
	const std::vector<size_t> entry_indices = { 0, 1, 2 };

	const auto worst = tree_source_t::worst_group_conflict({ entries, entry_indices, 1, false });

	REQUIRE(worst.worst_all == conflict_all_t::conflict);
}

TEST_CASE("tree_source_t::worst_group_conflict, hide duplicates single plugin contributes unknown", "[u]")
{
	const std::vector<conflict_entry_t> entries = { make_entry(
	    { make_version(4, conflict_this_t::override_wins), make_version(4, conflict_this_t::conflict_wins) }) };
	const std::vector<size_t> entry_indices = { 0 };

	const auto hidden = tree_source_t::worst_group_conflict({ entries, entry_indices, 4, true });
	REQUIRE(hidden.worst_this == conflict_this_t::unknown);

	const auto shown = tree_source_t::worst_group_conflict({ entries, entry_indices, 4, false });
	REQUIRE(shown.worst_this == conflict_this_t::override_wins);
}
