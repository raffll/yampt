#include <catch2/catch_all.hpp>
#include <patcher/patch_builder.hpp>
#include <rapidcheck/catch.h>
#include <session/plugin_session.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <QSignalSpy>
#include <rapidcheck.h>

namespace {

std::string make_sub_record(const std::string & sub_id, const std::string & content)
{
	std::string result;
	result += sub_id;
	const auto size_val = static_cast<uint32_t>(content.size());
	result.append(reinterpret_cast<const char *>(&size_val), 4);
	result += content;
	return result;
}

std::string make_record(const std::string & rec_id, const std::string & sub_records)
{
	std::string header;
	header += rec_id;
	const auto body_size = static_cast<uint32_t>(sub_records.size());
	header.append(reinterpret_cast<const char *>(&body_size), 4);
	header.append(8, '\0');
	return header + sub_records;
}

std::string make_tes3_header_record()
{
	return make_record("TES3", make_sub_record("HEDR", std::string(300, '\0')));
}

std::string make_activator_plugin_body(const std::string & record_id)
{
	const auto subs = make_sub_record("NAME", record_id + std::string(1, '\0')) +
	                  make_sub_record("FNAM", std::string("Signpost\0", 9));
	return make_tes3_header_record() + make_record("ACTI", subs);
}

std::string write_synthetic_plugin(const std::filesystem::path & dir, const std::string & filename)
{
	const auto path = (dir / filename).string();
	std::ofstream file(path, std::ios::binary);
	const auto body = make_activator_plugin_body("yampt_session_acti");
	file.write(body.data(), body.size());
	return path;
}

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
	    "patch plugins survive save/restore cycle",
	    []()
	{
		const auto patch = *gen_plugin_set();

		namespace fs = std::filesystem;
		const auto temp_path = fs::temp_directory_path() / "yampt_pbt_session.ini";
		const auto ini_path = QString::fromStdString(temp_path.string());

		plugin_session_t original;
		original.set_patch_plugins(patch);
		original.save_session_state(ini_path);

		plugin_session_t restored;
		restored.restore_session_state(ini_path);

		std::error_code remove_error;
		fs::remove(temp_path, remove_error);

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
	session.set_patch_plugins({ "test.esp" });
	session.unload_all();

	REQUIRE(session.load_source() == plugin_session_t::load_source_t::none);
	REQUIRE(session.load_base_path().empty());
	REQUIRE(session.scan().plugin_count() == 0);
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

TEST_CASE("plugin_session_t::load_from_folder, unloads previous folder before loading", "[u][qt]")
{
	namespace fs = std::filesystem;

	const auto folder_a = fs::temp_directory_path() / "yampt_session_reload_a";
	const auto folder_b = fs::temp_directory_path() / "yampt_session_reload_b";
	fs::create_directories(folder_a);
	fs::create_directories(folder_b);

	const auto path_a1 = write_synthetic_plugin(folder_a, "reload_a1.esm");
	const auto path_a2 = write_synthetic_plugin(folder_a, "reload_a2.esp");
	const auto path_b1 = write_synthetic_plugin(folder_b, "reload_b1.esm");

	plugin_session_t session;
	QSignalSpy unload_spy(&session, &plugin_session_t::plugins_unloaded);
	REQUIRE(unload_spy.isValid());

	session.load_from_folder({ path_a1, path_a2 }, folder_a.string());
	REQUIRE(session.scan().plugin_count() == 2);
	REQUIRE(unload_spy.count() == 0);

	session.load_from_folder({ path_b1 }, folder_b.string());
	REQUIRE(unload_spy.count() == 1);
	REQUIRE(session.scan().plugin_count() == 1);
	REQUIRE(session.scan().plugin_filename(0) == "reload_b1.esm");
	REQUIRE(session.load_base_path() == folder_b.string());

	std::error_code remove_error;
	fs::remove_all(folder_a, remove_error);
	fs::remove_all(folder_b, remove_error);
}
