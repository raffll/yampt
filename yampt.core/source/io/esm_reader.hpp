#pragma once

#include "../utility/includes.hpp"
#include "../utility/domain_types.hpp"

class esm_reader_t
{
public:
	static constexpr size_t record_header_size = 16;
	static constexpr size_t sub_record_header_size = 8;
	static constexpr size_t sub_record_id_size = 4;
	static constexpr size_t record_size_field_offset = 4;
	static constexpr size_t record_size_field_length = 4;

	void select_record(size_t index);
	void replace_record(const std::string & content);
	void set_modified(size_t index);

	void set_key(const std::string & sub_id);
	void set_value(const std::string & sub_id);
	void set_next_value(const std::string & sub_id);

	const auto & is_loaded()
	{
		return m_loaded;
	}

	const auto & get_name()
	{
		return m_name;
	}

	const auto & get_time()
	{
		return m_time;
	}

	const auto & get_records() const
	{
		return m_records;
	}

	const auto & get_record()
	{
		return *ptr_record;
	}

	size_t get_modified_count();

	struct sub_record_t
	{
		std::string sub_id;
		std::string content;
		std::string text;
		size_t pos = 0;
		size_t size = 0;
		size_t counter = 0;
		bool exist = false;
	};

	const auto & get_key()
	{
		return m_key;
	}

	const auto & get_value()
	{
		return m_value;
	}

	esm_reader_t() = default;
	esm_reader_t(const std::string & path);

private:
	void split_file(const std::string & content, const std::string & path);
	void set_time(const std::string & path);
	void scan_sub_records(size_t start_pos, sub_record_t & target);
	void mark_not_found(sub_record_t & target);
	void handle_exception(const std::exception & error);

	std::vector<record_t> m_records;
	file_path_parts_t m_name;
	std::filesystem::file_time_type m_time;
	bool m_loaded = false;

	record_t * ptr_record = nullptr;

	sub_record_t m_key;
	sub_record_t m_value;
};
