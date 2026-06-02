#pragma once

#include "includes.hpp"
#include "tools.hpp"

class esm_reader_t
{
public:
	void select_record(size_t i);
	void replace_record(const std::string & content);
	void set_modified(size_t i);

	void set_key(const std::string & id);
	void set_value(const std::string & id);
	void set_next_value(const std::string & id);

	const auto & is_loaded()
	{
		return loaded_;
	}

	const auto & get_name()
	{
		return name;
	}

	const auto & get_time()
	{
		return time;
	}

	const auto & get_records() const
	{
		return records;
	}

	const auto & get_record()
	{
		return *rec;
	}

	size_t get_modified_count();

	struct sub_record_t
	{
		std::string id;
		std::string content;
		std::string text;
		size_t pos = 0;
		size_t size = 0;
		size_t counter = 0;
		bool exist = false;
	};

	const auto & get_key()
	{
		return key;
	}

	const auto & get_value()
	{
		return value;
	}

	esm_reader_t() = default;
	esm_reader_t(const std::string & path);

private:
	void split_file(const std::string & content, const std::string & path);
	void set_time(const std::string & path);
	void main_loop(
	    std::size_t & cur_pos,
	    std::size_t & cur_size,
	    std::string & cur_id,
	    std::string & cur_text,
	    esm_reader_t::sub_record_t & subrecord);
	void handle_exception(const std::exception & e);

	std::vector<tools_t::record_t> records;
	tools_t::name_t name;
	std::filesystem::file_time_type time;
	bool loaded_ = false;

	tools_t::record_t * rec = nullptr;

	sub_record_t key;
	sub_record_t value;
};
