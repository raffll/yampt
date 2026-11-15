#pragma once

#include <functional>
#include <iostream>
#include <string>

class app_logger_t
{
public:
	using sink_fn_t = std::function<void(const std::string & entry)>;

	static void add_log(const std::string & entry, bool silent = false);
	static void set_sink(sink_fn_t sink_fn);
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

private:
	static std::string m_log;
	static bool m_error_flag;
	static bool m_debug_flag;
	static bool m_quiet_flag;
	static sink_fn_t m_sink_fn;
};
