#include "record_header_flags.hpp"
#include <cstring>

namespace record_header_flags
{
static bool intermediate_claims_bit(uint32_t first, uint32_t inter, uint32_t winner, uint32_t current, uint32_t mask)
{
	const bool inter_changed = (inter & mask) != (first & mask);
	const bool winner_unchanged = (winner & mask) == (first & mask);
	const bool current_unclaimed = (current & mask) == (first & mask);

	return inter_changed && winner_unchanged && current_unclaimed;
}

uint32_t read_flags(const std::string & content)
{
	if (content.size() < flags_offset + flags_length)
		return 0;

	uint32_t flags = 0;
	std::memcpy(&flags, content.data() + flags_offset, flags_length);

	return flags;
}

uint32_t merge_flags(const std::vector<std::string> & versions)
{
	if (versions.size() < 3)
		return read_flags(versions.empty() ? std::string() : versions.back());

	const uint32_t first = read_flags(versions.front());
	const uint32_t winner = read_flags(versions.back());
	uint32_t merged = winner;

	for (size_t version_idx = versions.size() - 2; version_idx >= 1; --version_idx)
	{
		const uint32_t inter = read_flags(versions[version_idx]);
		for (int bit = 0; bit < 32; ++bit)
		{
			const uint32_t mask = 1u << bit;
			if (intermediate_claims_bit(first, inter, winner, merged, mask))
				merged = (merged & ~mask) | (inter & mask);
		}
	}

	return merged;
}

void write_flags(std::string & content, uint32_t flags)
{
	if (content.size() < flags_offset + flags_length)
		return;

	std::memcpy(content.data() + flags_offset, &flags, flags_length);
}
} // namespace record_header_flags
