#include "interface/user_interface.hpp"
#include <io/binary_file_io.hpp>
#include <utility/app_logger.hpp>
#include <cstdlib>
#include <filesystem>

int main(int argc, char * argv[])
{
	auto exe_path = std::filesystem::path(argv[0]).parent_path();
	if (exe_path.empty())
		exe_path = std::filesystem::current_path();

	std::string base_dir;
#ifndef _WIN32
	const auto home_env = std::getenv("HOME");
	if (home_env)
		base_dir = (std::filesystem::path(home_env) / ".yampt").string();
	else
		base_dir = exe_path.string();
#else
	base_dir = exe_path.string();
#endif

	try
	{
		std::vector<std::string> arg;
		for (int i = 0; i < argc; i++)
		{
			arg.push_back(argv[i]);
		}
		user_interface_t ui(arg, base_dir);
	}
	catch (const std::exception & e)
	{
		app_logger_t::add_log("[error] " + std::string(e.what()) + "\r\n");
	}
	catch (...)
	{
		app_logger_t::add_log("[error] unknown error\r\n");
	}

	auto log_dir = std::filesystem::path(base_dir);
	std::filesystem::create_directories(log_dir);

	auto now = std::chrono::system_clock::now();
	auto time = std::chrono::system_clock::to_time_t(now);
	std::tm tm {};
#ifdef _WIN32
	localtime_s(&tm, &time);
#else
	localtime_r(&time, &tm);
#endif
	char time_str[32];
	std::strftime(time_str, sizeof(time_str), "%Y%m%d_%H%M%S", &tm);
	auto log_path = log_dir / (std::string("yampt_") + time_str + ".log");

	binary_file_io::write_text(app_logger_t::get_log(), log_path.string());
	return app_logger_t::has_error() ? 1 : 0;
}
