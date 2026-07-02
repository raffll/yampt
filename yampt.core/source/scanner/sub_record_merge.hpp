#pragma once

#include <string>
#include <vector>

struct merge_input_t
{
	std::string rec_type;
	std::string record_id;
	std::vector<std::string> version_contents;
};

struct merge_result_t
{
	bool changed = false;
	std::string content;
};

class sub_record_merge_t
{
public:
	static merge_result_t merge(const merge_input_t & input);

private:
	static merge_result_t merge_generic(const merge_input_t & input);
	static merge_result_t merge_cell_refs(const merge_input_t & input);
	static merge_result_t merge_element_wise(const merge_input_t & input);
	static bool needs_element_wise(const std::string & rec_type, const std::string & sub_type, size_t data_size);
	static std::string merge_bytes_three_way(const char * first, const char * inter, const char * winner, size_t size);
	static std::string merge_enam_slots(
		const std::vector<std::string> & first_enams,
		const std::vector<std::string> & inter_enams,
		const std::vector<std::string> & winner_enams);
};
