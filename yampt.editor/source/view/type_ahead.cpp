#include "type_ahead.hpp"
#include <utility/string_utils.hpp>

namespace type_ahead {

int next_matching_row(const std::vector<std::string> & filenames, const std::string & lowered_prefix, int start_row)
{
	const int total = static_cast<int>(filenames.size());
	if (total == 0)
		return -1;

	const int begin = (start_row + 1) % total;

	for (int offset = 0; offset < total; ++offset)
	{
		const int row = (begin + offset) % total;
		const auto & lowered = string_utils::to_lower(filenames[row]);
		if (lowered.starts_with(lowered_prefix))
			return row;
	}

	return -1;
}

}
