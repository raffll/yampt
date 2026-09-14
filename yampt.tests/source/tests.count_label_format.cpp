#include <catch2/catch_all.hpp>
#include <rapidcheck/catch.h>
#include <view/count_label_format.hpp>
#include <rapidcheck.h>

TEST_CASE("count_label_format::format, status label Lua count format", "[u][pbt]")
{
	rc::prop(
	    "the status label renders the plugin and record counts in the expected format",
	    []()
	{
		const auto plugins = *rc::gen::inRange<size_t>(0, 100);
		const auto records = *rc::gen::inRange<size_t>(0, 10000);
		const auto conflicts = *rc::gen::inRange<size_t>(0, 500);
		const auto lua_conflicts = *rc::gen::inRange<size_t>(0, 200);

		const auto result = count_label_format::format(plugins, records, conflicts, lua_conflicts);

		if (lua_conflicts > 0)
		{
			const auto expected_substring = QString("%1 Lua conflicts").arg(lua_conflicts);
			RC_ASSERT(result.contains(expected_substring));
		}
		else
		{
			RC_ASSERT(!result.contains("Lua"));
		}
	});
}
