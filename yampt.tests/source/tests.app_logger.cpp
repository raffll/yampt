#include <catch2/catch_all.hpp>
#include <utility/app_logger.hpp>

TEST_CASE("app_logger_t::add_log, accumulates messages", "[u]")
{
	app_logger_t::reset_log();
	app_logger_t::add_log("[info] first\r\n");
	app_logger_t::add_log("[info] second\r\n");
	REQUIRE(app_logger_t::get_log() == "[info] first\r\n[info] second\r\n");
}

TEST_CASE("app_logger_t::reset_log, clears log and error flag", "[u]")
{
	app_logger_t::add_log("[error] something\r\n");
	REQUIRE(app_logger_t::has_error() == true);
	app_logger_t::reset_log();
	REQUIRE(app_logger_t::get_log().empty());
	REQUIRE(app_logger_t::has_error() == false);
}

TEST_CASE("app_logger_t::add_log, sets error flag on error prefix", "[u]")
{
	app_logger_t::reset_log();
	app_logger_t::add_log("[info] normal\r\n");
	REQUIRE(app_logger_t::has_error() == false);
	app_logger_t::add_log("[error] bad thing\r\n");
	REQUIRE(app_logger_t::has_error() == true);
}

TEST_CASE("app_logger_t::add_log, silent messages hidden when debug off", "[u]")
{
	app_logger_t::reset_log();
	app_logger_t::set_debug(false);
	app_logger_t::add_log("[debug] hidden\r\n", true);
	REQUIRE(app_logger_t::get_log().empty());
}

TEST_CASE("app_logger_t::add_log, silent messages visible when debug on", "[u]")
{
	app_logger_t::reset_log();
	app_logger_t::set_debug(true);
	app_logger_t::add_log("[debug] visible\r\n", true);
	REQUIRE(app_logger_t::get_log() == "[debug] visible\r\n");
	app_logger_t::set_debug(false);
}

TEST_CASE("app_logger_t::set_exe_dir, round-trip", "[u]")
{
	app_logger_t::set_exe_dir("/some/path");
	REQUIRE(app_logger_t::get_exe_dir() == "/some/path");
	app_logger_t::set_exe_dir("");
}
