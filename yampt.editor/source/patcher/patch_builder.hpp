#pragma once

#include <cstdint>
#include <string>
#include <vector>

class patch_builder_t
{
public:
	struct merge_record_t
	{
		std::string rec_type;
		std::string record_id;
		std::string content;
	};

	void clear();

	void add_record(const std::string & rec_type, const std::string & record_id, const std::string & content);
	void add_record_raw(const std::string & rec_type, const std::string & record_id, const std::string & content);
	const std::string * find_content(const std::string & rec_type, const std::string & record_id) const;
	void remove_record(const std::string & type, const std::string & id);

	bool has_records() const;
	size_t record_count() const;
	const std::string & record_content(size_t index) const;
	const std::string & record_type(size_t index) const;
	const std::string & record_id(size_t index) const;

	struct leveled_list_input_t
	{
		std::string rec_type;
		std::string record_id;
		std::vector<std::string> version_contents;
	};

	void merge_leveled_list(const leveled_list_input_t & input);
	void merge_dialogue(const leveled_list_input_t & input);

	struct master_entry_t
	{
		std::string filename;
		uint64_t file_size = 0;
	};

	bool save(
	    const std::string & output_path,
	    const std::string & author,
	    const std::string & description,
	    const std::vector<master_entry_t> & masters);

	static std::string build_tes3_header(
	    const std::string & author,
	    const std::string & description,
	    size_t record_count,
	    const std::vector<master_entry_t> & masters);

private:
	std::vector<merge_record_t> m_records;
};
