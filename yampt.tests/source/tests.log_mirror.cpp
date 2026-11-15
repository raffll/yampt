#include <catch2/catch_all.hpp>
#include <launch_options.hpp>
#include <log_mirror.hpp>
#include <utility/app_logger.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <QDir>

TEST_CASE("launch_options::parse, no arguments keeps console and debug off", "[u]")
{
	const auto options = launch_options::parse({ "yEditor.exe" });
	REQUIRE_FALSE(options.console);
	REQUIRE_FALSE(options.debug);
}

TEST_CASE("launch_options::parse, recognizes console and debug flags", "[u]")
{
	const auto options = launch_options::parse({ "yEditor.exe", "--debug", "--console" });
	REQUIRE(options.console);
	REQUIRE(options.debug);
}

TEST_CASE("launch_options::parse, ignores unknown arguments", "[u]")
{
	const auto options = launch_options::parse({ "yEditor.exe", "--verbose", "debug" });
	REQUIRE_FALSE(options.console);
	REQUIRE_FALSE(options.debug);
}

TEST_CASE("log_mirror_t::stamp_lines, prefixes every non-empty line", "[u]")
{
	const auto stamped = log_mirror_t::stamp_lines("12:00:00.000", "[info] first\r\n\n[error] second");
	REQUIRE(stamped == "12:00:00.000 [info] first\n12:00:00.000 [error] second\n");
}

TEST_CASE("log_mirror_t::stamp_lines, empty text gives nothing", "[u]")
{
	REQUIRE(log_mirror_t::stamp_lines("12:00:00.000", "").empty());
	REQUIRE(log_mirror_t::stamp_lines("12:00:00.000", "\r\n").empty());
}

TEST_CASE("log_mirror_t::default_log_path, app log file in the temp folder", "[u]")
{
	const auto path = std::filesystem::path(log_mirror_t::default_log_path("yEditor"));
	REQUIRE(path.filename() == "yEditor.log");
	REQUIRE(std::filesystem::equivalent(path.parent_path(), std::filesystem::path(QDir::tempPath().toStdString())));
}

TEST_CASE("log_mirror_t::write, appends stamped lines to a fresh log file", "[i]")
{
	const auto log_path = (std::filesystem::temp_directory_path() / "yampt_log_mirror_test.log").string();
	{
		std::ofstream stale(log_path);
		stale << "stale content from an earlier run\n";
	}

	auto & mirror = log_mirror_t::instance();
	mirror.start({ log_path, false });
	mirror.write("[info] loaded plugin\n");
	mirror.write("[warning] missing master\n");

	std::ifstream file(log_path);
	std::stringstream content;
	content << file.rdbuf();
	const auto text = content.str();

	REQUIRE(text.find("stale content") == std::string::npos);
	REQUIRE(text.find(" [info] loaded plugin\n") != std::string::npos);
	REQUIRE(text.find(" [warning] missing master\n") != std::string::npos);
}

TEST_CASE("app_logger_t::set_sink, receives shown entries and gated debug entries only when debug is on", "[u]")
{
	std::vector<std::string> received;
	app_logger_t::set_quiet(true);
	app_logger_t::set_sink([&received](const std::string & entry) { received.push_back(entry); });

	app_logger_t::set_debug(false);
	app_logger_t::add_log("[info] shown\n");
	app_logger_t::add_log("[debug] hidden\n", true);
	app_logger_t::set_debug(true);
	app_logger_t::add_log("[debug] visible\n", true);

	app_logger_t::set_debug(false);
	app_logger_t::set_sink({});
	app_logger_t::set_quiet(false);
	app_logger_t::reset_log();

	REQUIRE(received == std::vector<std::string> { "[info] shown\n", "[debug] visible\n" });
}
