#include "loc_inflection_annotations.hpp"

namespace loc_inflection_annotations {

std::vector<annotation_t> build(
    loc_types::loc_file_kind_t file_kind,
    const std::vector<loc_types::loc_entry_t> & entries)
{
	std::vector<annotation_t> result;

	if (file_kind == loc_types::loc_file_kind_t::cel)
		return result;

	result.reserve(entries.size());
	for (const auto & entry : entries)
	{
		annotation_t annotation;
		annotation.start = 0;
		annotation.end = 0;
		annotation.kind = annotation_t::inflection_form;
		annotation.old_text = entry.key;
		annotation.new_text = entry.value;
		result.push_back(std::move(annotation));
	}

	return result;
}

} // namespace loc_inflection_annotations
