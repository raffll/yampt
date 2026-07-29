#include "binary_file_io.hpp"
#include "../utility/app_logger.hpp"
#include <fstream>

static constexpr size_t read_buffer_size = 16384;

std::string binary_file_io::read_file(const std::string & path)
{
	std::string content;
	std::ifstream file(path, std::ios::binary);
	if (file)
	{
		app_logger_t::add_log("[info] loading \"" + path + "\"\r\n");
		char buffer[read_buffer_size];
		file.seekg(0, std::ios::end);
		auto file_size = file.tellg();
		if (file_size == 0)
		{
			app_logger_t::add_log("[warning] file is empty: \"" + path + "\"\r\n");
			return content;
		}
		content.reserve(static_cast<size_t>(file_size));
		file.seekg(0, std::ios::beg);
		std::streamsize chars_read;
		while (file.read(buffer, sizeof(buffer)), chars_read = file.gcount())
		{
			content.append(buffer, chars_read);
		}
	}
	else
	{
		app_logger_t::add_log("[error] cannot load \"" + path + "\" (wrong path)\r\n");
	}
	return content;
}

void binary_file_io::write_text(const std::string & text, const std::string & path)
{
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		app_logger_t::add_log("[error] cannot open \"" + path + "\" for writing\r\n");
		return;
	}
	file << text;
	app_logger_t::add_log("[info] writing \"" + path + "\"\r\n");
}

void binary_file_io::write_file(const std::vector<record_t> & records, const std::string & path)
{
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		app_logger_t::add_log("[error] cannot open \"" + path + "\" for writing\r\n");
		return;
	}
	for (const auto & record : records)
	{
		file << record.content;
	}
	app_logger_t::add_log("[info] writing \"" + path + "\"\r\n");
}

void binary_file_io::create_file(const std::vector<record_t> & records, const std::string & path)
{
	std::ofstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		app_logger_t::add_log("[error] cannot open \"" + path + "\" for writing\r\n");
		return;
	}
	for (size_t i = 0; i < records.size(); ++i)
	{
		const auto & record = records.at(i);
		if (!record.modified)
			continue;

		file << record.content;
	}
	app_logger_t::add_log("[info] writing \"" + path + "\"\r\n");
}
