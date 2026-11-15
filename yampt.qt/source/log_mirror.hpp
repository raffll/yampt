#pragma once

#include <fstream>
#include <mutex>
#include <string>

struct log_mirror_config_t
{
	std::string log_path;
	bool console = false;
};

class log_mirror_t
{
public:
	static log_mirror_t & instance();

	static std::string default_log_path(const std::string & app_name);
	static std::string stamp_lines(const std::string & time_stamp, const std::string & text);

	void start(const log_mirror_config_t & config);
	void write(const std::string & text);

private:
	std::ofstream m_file;
	bool m_console = false;
	std::mutex m_write_mutex;
};
