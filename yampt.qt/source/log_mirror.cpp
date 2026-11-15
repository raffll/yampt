#include "log_mirror.hpp"
#include <utility/string_utils.hpp>
#include <cstdio>
#include <iostream>
#include <QDir>
#include <QTime>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

void attach_console()
{
#ifdef _WIN32
	if (!AttachConsole(ATTACH_PARENT_PROCESS))
		AllocConsole();

	SetConsoleOutputCP(CP_UTF8);
	FILE * console_stream = nullptr;
	freopen_s(&console_stream, "CONOUT$", "w", stdout);
	std::cout.clear();
#endif
}

std::string current_time_stamp()
{
	return QTime::currentTime().toString("HH:mm:ss.zzz").toStdString();
}

} // namespace

log_mirror_t & log_mirror_t::instance()
{
	static log_mirror_t mirror;
	return mirror;
}

std::string log_mirror_t::default_log_path(const std::string & app_name)
{
	const auto file_name = QString::fromStdString(app_name + ".log");
	return QDir(QDir::tempPath()).filePath(file_name).toUtf8().toStdString();
}

std::string log_mirror_t::stamp_lines(const std::string & time_stamp, const std::string & text)
{
	std::string stamped;
	size_t line_start = 0;
	while (line_start < text.size())
	{
		const auto line_end = text.find('\n', line_start);
		const auto stop = (line_end == std::string::npos) ? text.size() : line_end;
		auto line = text.substr(line_start, stop - line_start);
		if (!line.empty() && line.back() == '\r')
			line.pop_back();

		if (!line.empty())
			stamped += time_stamp + " " + line + "\n";

		line_start = stop + 1;
	}

	return stamped;
}

void log_mirror_t::start(const log_mirror_config_t & config)
{
	const std::lock_guard<std::mutex> lock(m_write_mutex);
	m_file.open(string_utils::utf8_to_path(config.log_path), std::ios::binary | std::ios::trunc);
	m_console = config.console;
	if (m_console)
		attach_console();
}

void log_mirror_t::write(const std::string & text)
{
	const auto stamped = stamp_lines(current_time_stamp(), text);
	if (stamped.empty())
		return;

	const std::lock_guard<std::mutex> lock(m_write_mutex);
	if (m_file.is_open())
		m_file << stamped << std::flush;

	if (m_console)
		std::cout << stamped << std::flush;
}
