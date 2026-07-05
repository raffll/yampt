#include <catch2/catch_all.hpp>
#include <rapidcheck/catch.h>
#include <rapidcheck.h>
#include <model/nav_tree_filter.hpp>
#include <scanner/plugin_scan.hpp>

namespace rc {

template <>
struct Arbitrary<conflict_all_t>
{
	static Gen<conflict_all_t> arbitrary()
	{
		return gen::element(
			conflict_all_t::unknown,
			conflict_all_t::only_one,
			conflict_all_t::no_conflict,
			conflict_all_t::override_benign,
			conflict_all_t::conflict);
	}
};

template <>
struct Arbitrary<conflict_this_t>
{
	static Gen<conflict_this_t> arbitrary()
	{
		return gen::element(
			conflict_this_t::unknown,
			conflict_this_t::master,
			conflict_this_t::identical_to_master,
			conflict_this_t::override_wins,
			conflict_this_t::conflict_wins,
			conflict_this_t::conflict_loses,
			conflict_this_t::deleted);
	}
};

}

namespace {

conflict_entry_t make_random_entry()
{
	conflict_entry_t entry;
	entry.conflict_all = *rc::gen::arbitrary<conflict_all_t>();
	entry.rec_type = *rc::gen::element(std::string("NPC_"), std::string("CELL"), std::string("ARMO"), std::string("WEAP"), std::string("BOOK"), std::string("DIAL"));
	entry.record_id = *rc::gen::arbitrary<std::string>();
	entry.display_name = *rc::gen::arbitrary<std::string>();

	const auto version_count = *rc::gen::inRange(1, 5);
	for (int index = 0; index < version_count; ++index)
	{
		record_version_t version;
		version.plugin_idx = index;
		version.record_index = static_cast<size_t>(*rc::gen::inRange(0, 1000));
		version.status = *rc::gen::arbitrary<conflict_this_t>();
		entry.versions.push_back(version);
	}

	return entry;
}

nav_tree_filter_t::filter_state_t make_random_filter(const conflict_entry_t & entry)
{
	nav_tree_filter_t::filter_state_t state;

	state.filter_conflict_all = *rc::gen::arbitrary<bool>();
	if (state.filter_conflict_all)
	{
		const auto count = *rc::gen::inRange(1, 4);
		for (int index = 0; index < count; ++index)
			state.conflict_all_set.insert(*rc::gen::arbitrary<conflict_all_t>());
	}

	state.filter_conflict_this = *rc::gen::arbitrary<bool>();
	if (state.filter_conflict_this)
	{
		const auto count = *rc::gen::inRange(1, 4);
		for (int index = 0; index < count; ++index)
			state.conflict_this_set.insert(*rc::gen::arbitrary<conflict_this_t>());
	}

	state.filter_by_type = *rc::gen::arbitrary<bool>();
	if (state.filter_by_type)
	{
		const auto count = *rc::gen::inRange(1, 3);
		for (int index = 0; index < count; ++index)
			state.type_set.insert(*rc::gen::element(std::string("NPC_"), std::string("CELL"), std::string("ARMO"), std::string("WEAP"), std::string("BOOK"), std::string("DIAL")));
	}

	state.filter_by_id = *rc::gen::arbitrary<bool>();
	if (state.filter_by_id)
		state.id_text = *rc::gen::nonEmpty<std::string>();

	state.filter_by_name = *rc::gen::arbitrary<bool>();
	if (state.filter_by_name)
		state.name_text = *rc::gen::nonEmpty<std::string>();

	state.filter_deleted = *rc::gen::arbitrary<bool>();

	return state;
}

bool passes_single_dimension_conflict_all(const conflict_entry_t & entry, const nav_tree_filter_t::filter_state_t & full)
{
	nav_tree_filter_t filter;
	nav_tree_filter_t::filter_state_t single;
	single.filter_conflict_all = full.filter_conflict_all;
	single.conflict_all_set = full.conflict_all_set;
	filter.set_filter(single);
	return filter.passes(entry, 0);
}

bool passes_single_dimension_conflict_this(const conflict_entry_t & entry, const nav_tree_filter_t::filter_state_t & full)
{
	nav_tree_filter_t filter;
	nav_tree_filter_t::filter_state_t single;
	single.filter_conflict_this = full.filter_conflict_this;
	single.conflict_this_set = full.conflict_this_set;
	filter.set_filter(single);
	return filter.passes(entry, 0);
}

bool passes_single_dimension_type(const conflict_entry_t & entry, const nav_tree_filter_t::filter_state_t & full)
{
	nav_tree_filter_t filter;
	nav_tree_filter_t::filter_state_t single;
	single.filter_by_type = full.filter_by_type;
	single.type_set = full.type_set;
	filter.set_filter(single);
	return filter.passes(entry, 0);
}

bool passes_single_dimension_id(const conflict_entry_t & entry, const nav_tree_filter_t::filter_state_t & full)
{
	nav_tree_filter_t filter;
	nav_tree_filter_t::filter_state_t single;
	single.filter_by_id = full.filter_by_id;
	single.id_text = full.id_text;
	filter.set_filter(single);
	return filter.passes(entry, 0);
}

bool passes_single_dimension_name(const conflict_entry_t & entry, const nav_tree_filter_t::filter_state_t & full)
{
	nav_tree_filter_t filter;
	nav_tree_filter_t::filter_state_t single;
	single.filter_by_name = full.filter_by_name;
	single.name_text = full.name_text;
	filter.set_filter(single);
	return filter.passes(entry, 0);
}

bool passes_single_dimension_deleted(const conflict_entry_t & entry, const nav_tree_filter_t::filter_state_t & full)
{
	nav_tree_filter_t filter;
	nav_tree_filter_t::filter_state_t single;
	single.filter_deleted = full.filter_deleted;
	filter.set_filter(single);
	return filter.passes(entry, 0);
}

}

TEST_CASE("nav_tree_filter_t::passes, AND-composition", "[u][pbt]")
{
	rc::prop(
		"combined filter equals AND of each dimension independently",
		[]()
	{
		const auto entry = make_random_entry();
		const auto filter_state = make_random_filter(entry);

		nav_tree_filter_t combined;
		combined.set_filter(filter_state);
		const auto combined_result = combined.passes(entry, 0);

		const auto expected = passes_single_dimension_conflict_all(entry, filter_state)
			&& passes_single_dimension_conflict_this(entry, filter_state)
			&& passes_single_dimension_type(entry, filter_state)
			&& passes_single_dimension_id(entry, filter_state)
			&& passes_single_dimension_name(entry, filter_state)
			&& passes_single_dimension_deleted(entry, filter_state);

		RC_ASSERT(combined_result == expected);
	});
}
