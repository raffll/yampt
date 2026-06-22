#include "includes.hpp"
#include "tools.hpp"
#include "user_interface.hpp"

int main(int argc, char * argv[])
{
	auto exe_path = std::filesystem::path(argv[0]).parent_path();
	if (exe_path.empty())
		exe_path = std::filesystem::current_path();

	tools_t::set_exe_dir(exe_path.string());

	try
	{
		std::vector<std::string> arg;
		for (int i = 0; i < argc; i++)
		{
			arg.push_back(argv[i]);
		}
		user_interface_t ui(arg);
	}
	catch (const std::exception & e)
	{
		tools_t::add_log("[error] " + std::string(e.what()) + "\r\n");
	}
	catch (...)
	{
		tools_t::add_log("[error] unknown error\r\n");
	}

	auto exe_dir = std::filesystem::path(argv[0]).parent_path();
	if (exe_dir.empty())
		exe_dir = std::filesystem::current_path();

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
	auto log_path = exe_dir / (std::string("yampt_") + time_str + ".log");

	tools_t::write_text(tools_t::get_log(), log_path.string());
	return tools_t::has_error() ? 1 : 0;
}
