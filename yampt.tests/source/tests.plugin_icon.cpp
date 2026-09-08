#include <catch2/catch_all.hpp>
#include <view/plugin_icon.hpp>

namespace {

constexpr auto gear = "\xE2\x9A\x99";
constexpr auto scroll = "\xF0\x9F\x93\x9C";
constexpr auto page = "\xF0\x9F\x93\x84";
constexpr auto bolt = "\xE2\x9A\xA1";
constexpr auto no_entry = "\xF0\x9F\x9A\xAB";
constexpr auto shield = "\xF0\x9F\x9B\xA1";
constexpr auto star = "\xE2\xAD\x90";

QString build(const plugin_icon::tier_flags_t & flags)
{
	return plugin_icon::prefix(flags);
}

} // namespace

TEST_CASE("plugin_icon::has_esm_extension, true only for .esm suffix", "[u]")
{
	REQUIRE(plugin_icon::has_esm_extension("Morrowind.esm"));
	REQUIRE(plugin_icon::has_esm_extension("Morrowind.ESM"));
	REQUIRE_FALSE(plugin_icon::has_esm_extension("Morrowind.esp"));
	REQUIRE_FALSE(plugin_icon::has_esm_extension(".esm"));
	REQUIRE_FALSE(plugin_icon::has_esm_extension("esm"));
}

TEST_CASE("plugin_icon::path_is_overwrite, detects overwrite folder in either separator", "[u]")
{
	REQUIRE(plugin_icon::path_is_overwrite("C:/mods/overwrite/plugin.esp"));
	REQUIRE(plugin_icon::path_is_overwrite("C:\\mods\\overwrite\\plugin.esp"));
	REQUIRE_FALSE(plugin_icon::path_is_overwrite("C:/mods/normal/plugin.esp"));
	REQUIRE_FALSE(plugin_icon::path_is_overwrite("C:/overwrite.esp"));
}

TEST_CASE("plugin_icon::prefix, merged patch shows gear base tier", "[u]")
{
	plugin_icon::tier_flags_t flags;
	flags.filename = merged_patch::filename;

	const auto result = build(flags);
	REQUIRE(result.startsWith(QString::fromUtf8(gear)));
	REQUIRE_FALSE(result.contains(QString::fromUtf8(scroll)));
	REQUIRE_FALSE(result.contains(QString::fromUtf8(page)));
}

TEST_CASE("plugin_icon::prefix, master shows scroll base tier", "[u]")
{
	plugin_icon::tier_flags_t flags;
	flags.filename = "Tribunal.esm";

	REQUIRE(build(flags).startsWith(QString::fromUtf8(scroll)));
}

TEST_CASE("plugin_icon::prefix, regular plugin shows page base tier", "[u]")
{
	plugin_icon::tier_flags_t flags;
	flags.filename = "MyMod.esp";

	REQUIRE(build(flags).startsWith(QString::fromUtf8(page)));
}

TEST_CASE("plugin_icon::prefix, exclude and guard are mutually exclusive with exclude winning", "[u]")
{
	plugin_icon::tier_flags_t flags;
	flags.filename = "MyMod.esp";
	flags.is_excluded = true;
	flags.is_guard = true;

	const auto result = build(flags);
	REQUIRE(result.contains(QString::fromUtf8(no_entry)));
	REQUIRE_FALSE(result.contains(QString::fromUtf8(shield)));
}

TEST_CASE("plugin_icon::prefix, guard shield shows when not excluded", "[u]")
{
	plugin_icon::tier_flags_t flags;
	flags.filename = "MyMod.esp";
	flags.is_guard = true;

	REQUIRE(build(flags).contains(QString::fromUtf8(shield)));
}

TEST_CASE("plugin_icon::prefix, all tiers append in fixed order", "[u]")
{
	plugin_icon::tier_flags_t flags;
	flags.filename = merged_patch::filename;
	flags.is_overridden = true;
	flags.is_excluded = true;
	flags.is_active = true;

	const auto result = build(flags);
	const int gear_pos = result.indexOf(QString::fromUtf8(gear));
	const int bolt_pos = result.indexOf(QString::fromUtf8(bolt));
	const int no_entry_pos = result.indexOf(QString::fromUtf8(no_entry));
	const int star_pos = result.indexOf(QString::fromUtf8(star));

	REQUIRE(gear_pos >= 0);
	REQUIRE(gear_pos < bolt_pos);
	REQUIRE(bolt_pos < no_entry_pos);
	REQUIRE(no_entry_pos < star_pos);
}

TEST_CASE("plugin_icon::prefix, active star omitted when inactive", "[u]")
{
	plugin_icon::tier_flags_t flags;
	flags.filename = "MyMod.esp";
	flags.is_active = false;

	REQUIRE_FALSE(build(flags).contains(QString::fromUtf8(star)));
}

TEST_CASE("plugin_icon::prefix, plain plugin emits only base tier and trailing space", "[u]")
{
	plugin_icon::tier_flags_t flags;
	flags.filename = "MyMod.esp";

	REQUIRE(build(flags) == QString::fromUtf8(page) + " ");
}
