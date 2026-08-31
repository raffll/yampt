#include "case_fold.hpp"

namespace case_fold {

std::uint32_t to_lower(std::uint32_t code_point)
{
	if (code_point >= 'A' && code_point <= 'Z')
		return code_point + 0x20;

	if (code_point >= 0x00C0 && code_point <= 0x00DE && code_point != 0x00D7)
		return code_point + 0x20;

	if (code_point >= 0x0139 && code_point <= 0x0148)
	{
		if (code_point % 2 == 1)
			return code_point + 1;

		return code_point;
	}

	if (code_point >= 0x0100 && code_point <= 0x0177)
	{
		if (code_point % 2 == 0)
			return code_point + 1;

		return code_point;
	}

	if (code_point == 0x0178)
		return 0x00FF;

	if (code_point >= 0x0179 && code_point <= 0x017E)
	{
		if (code_point % 2 == 1)
			return code_point + 1;

		return code_point;
	}

	if (code_point >= 0x0410 && code_point <= 0x042F)
		return code_point + 0x20;

	if (code_point == 0x0401)
		return 0x0451;

	return code_point;
}

} // namespace case_fold
