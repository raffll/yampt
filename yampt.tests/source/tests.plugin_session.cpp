#include <catch2/catch_all.hpp>
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

TEST_CASE("plugin_session_t, persistence round-trip", "[pbt]")
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
