#include <catch2/catch_all.hpp>
#include <patcher/patch_builder.hpp>
#include <rapidcheck/catch.h>
#include <session/plugin_session.hpp>
#include <filesystem>
#include <rapidcheck.h>

namespace {

rc::Gen<std::string> gen_plugin_name()
{
	return rc::gen::exec([]()
	{
		const auto length = *rc::gen::inRange(3, 12);
		std::string name;
		name.reserve(length + 4);
		for (int index = 0; index < length; ++index)
			name += *rc::gen::inRange('a', 'z');

		name += ".esp";
		return name;
	});
}

rc::Gen<std::set<std::string>> gen_plugin_set()
{
	return rc::gen::container<std::set<std::string>>(gen_plugin_name());
}

} // namespace

TEST_CASE("plugin_session_t::save, persistence round-trip", "[pbt]")
{
	rc::prop(
	    "excluded and patch plugins survive save/restore cycle",
	    []()
	{
		const auto excluded = *gen_plugin_set();
		const auto patch = *gen_plugin_set();

		namespace fs = std::filesystem;
		const auto temp_path = fs::temp_directory_path() / "yampt_pbt_session.ini";
		const auto ini_path = QString::fromStdString(temp_path.string());

		plugin_session_t original;
		original.set_excluded_plugins(excluded);
		original.set_patch_plugins(patch);
		original.save_session_state(ini_path);

		plugin_session_t restored;
		restored.restore_session_state(ini_path);

		std::error_code remove_error;
		fs::remove(temp_path, remove_error);

		RC_ASSERT(restored.excluded_plugins() == excluded);
		RC_ASSERT(restored.patch_plugins() == patch);
	});
}

TEST_CASE("plugin_session_t::load_source, none by default", "[u][qt]")
{
	plugin_session_t session;
	REQUIRE(session.load_source() == plugin_session_t::load_source_t::none);
	REQUIRE(session.load_base_path().empty());
}

TEST_CASE("plugin_session_t::scan, empty before load", "[u][qt]")
{
	plugin_session_t session;
	REQUIRE(session.scan().plugin_count() == 0);
}

TEST_CASE("plugin_session_t::unload_all, resets state", "[u][qt]")
{
	plugin_session_t session;
	session.set_excluded_plugins({ "test.esp" });
	session.unload_all();

	REQUIRE(session.load_source() == plugin_session_t::load_source_t::none);
	REQUIRE(session.load_base_path().empty());
	REQUIRE(session.scan().plugin_count() == 0);
}

TEST_CASE("plugin_session_t::excluded_plugins, round-trip", "[u][qt]")
{
	plugin_session_t session;
	std::set<std::string> excluded = { "mod_a.esp", "mod_b.esp" };
	session.set_excluded_plugins(excluded);

	REQUIRE(session.excluded_plugins() == excluded);
}

TEST_CASE("plugin_session_t::patch_plugins, round-trip", "[u][qt]")
{
	plugin_session_t session;
	std::set<std::string> patch = { "patch_one.esp", "patch_two.esp" };
	session.set_patch_plugins(patch);

	REQUIRE(session.patch_plugins() == patch);
}

TEST_CASE("plugin_session_t::patch_builder, accessible and empty", "[u][qt]")
{
	plugin_session_t session;
	REQUIRE_FALSE(session.patch_builder().has_records());
	REQUIRE(session.patch_builder().record_count() == 0);
}
