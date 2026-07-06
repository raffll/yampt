#pragma once

#include <iostream>
#include <string>

class app_logger_t
{
public:
	static void add_log(const std::string & entry, bool silent = false);
	static void reset_log();

	static const std::string & get_log()
	{
		return m_log;
	}

	static bool has_error()
	{
		return m_error_flag;
	}

	static void set_debug(bool enabled)
	{
		m_debug_flag = enabled;
	}

	static bool is_debug()
	{
		return m_debug_flag;
	}

	static void set_quiet(bool enabled)
	{
		m_quiet_flag = enabled;
	}

	static void set_exe_dir(const std::string & dir)
	{
		m_exe_dir = dir;
	}

	static const std::string & get_exe_dir()
	{
		return m_exe_dir;
	}

private:
	static std::string m_log;
	static bool m_error_flag;
	static bool m_debug_flag;
	static bool m_quiet_flag;
	static std::string m_exe_dir;
};
