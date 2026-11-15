#pragma once

#include "merge_patch_store.hpp"
#include <functional>
#include <optional>
#include <string>
#include <vector>

class plugin_scan_t;

namespace dialogue_arranger {

using find_topic_dial_fn_t = std::function<std::optional<std::string>(const std::string & topic)>;

struct arrange_input_t
{
	const std::vector<merge_record_t> & records;
	find_topic_dial_fn_t find_topic_dial;
};

struct arrange_result_t
{
	std::vector<merge_record_t> records;
	std::vector<std::string> inserted_topics;
	std::vector<std::string> missing_topics;
};

std::string topic_of_info(const std::string & info_record_id);
arrange_result_t arrange(const arrange_input_t & input);
arrange_result_t arrange_active(plugin_scan_t & scan);

} // namespace dialogue_arranger
