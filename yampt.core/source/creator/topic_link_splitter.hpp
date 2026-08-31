#pragma once

#include <string>
#include <vector>

struct link_segment_t
{
	bool is_link = false;
	std::string plain_text;
	std::string inner_phrase;
	std::string pseudo_asterisks;
};

namespace topic_link_splitter {

std::vector<link_segment_t> split(const std::string & line);
std::string reassemble(const std::vector<link_segment_t> & segments);

} // namespace topic_link_splitter
