#include <catch2/catch_all.hpp>
#include <model/nav_tree_filter.hpp>
#include <rapidcheck/catch.h>
#include <scanner/conflict_detector.hpp>
#include <rapidcheck.h>

namespace {

rc::Gen<conflict_severity_t> gen_severity()
{
	return rc::gen::element(
	    conflict_severity_t::blocking, conflict_severity_t::mutating, conflict_severity_t::overlapping);
}

rc::Gen<std::string> gen_interface_name()
{
	return rc::gen::element(
	    std::string("I.AI"),
	    std::string("I.Combat"),
	    std::string("I.Weather"),
	    std::string("I.UI"),
	    std::string("I.Dialogue"),
	    std::string("I.Magic"),
	    std::string("I.Movement"));
}

handler_conflict_t make_conflict(const std::string & interface_name, conflict_severity_t severity)
{
	handler_conflict_t conflict;
	conflict.interface_name = interface_name;
	conflict.method_name = "onUpdate";
	conflict.type_argument = "";
	conflict.severity = severity;
	return conflict;
}

} // namespace

TEST_CASE(
    "nav_tree_filter_t::passes_lua_conflict, interface name filter correctness",
    "[Feature: lua-view-integration][Property 7: Interface name filter correctness]")
{
	rc::prop(
	    "Validates: Requirements 3.3",
	    []()
	{
		const auto interface_name = *gen_interface_name();
		const auto severity = *gen_severity();
		const auto conflict = make_conflict(interface_name, severity);

		const auto allowed_count = *rc::gen::inRange(1, 5);
		std::set<std::string> allowed_set;
		for (int index = 0; index < allowed_count; ++index)
			allowed_set.insert(*gen_interface_name());

		nav_tree_filter_t::filter_state_t state;
		state.filter_lua_interface = true;
		state.lua_interface_set = allowed_set;

		nav_tree_filter_t filter;
		filter.set_filter(state);

		const auto result = filter.passes_lua_conflict(conflict);
		const auto expected = allowed_set.contains(interface_name);

		RC_ASSERT(result == expected);
	});
}
