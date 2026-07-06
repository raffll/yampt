#include "app_logger.hpp"

std::string app_logger_t::m_log;
bool app_logger_t::m_error_flag = false;
bool app_logger_t::m_debug_flag = false;
bool app_logger_t::m_quiet_flag = false;
std::string app_logger_t::m_exe_dir;

void app_logger_t::add_log(const std::string & entry, bool silent)
{
	if (entry.find("[error]") == 0)
	{
		m_error_flag = true;
	}

	if (silent && !m_debug_flag)
		return;

	m_log += entry;

	if (!silent && !m_quiet_flag)
		std::cout << entry;
}

void app_logger_t::reset_log()
{
	m_log.clear();
	m_error_flag = false;
}
