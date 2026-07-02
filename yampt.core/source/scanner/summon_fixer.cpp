#include "summon_fixer.hpp"
#include "../utility/string_utils.hpp"
#include <array>
#include <string_view>

static constexpr size_t flags_offset = 12;
static constexpr uint8_t persistent_flag_byte = 0x04;

static constexpr std::array<std::string_view, 22> known_summon_ids = {
    "ancestor_ghost_summon",
    "atronach_flame_summon",
    "atronach_frost_summon",
    "atronach_storm_summon",
    "bonelord_summon",
    "bonewalker_summon",
    "bonewalker_greater_summ",
    "centurion_sphere_summon",
    "clannfear_summon",
    "daedroth_summon",
    "dremora_summon",
    "golden saint_summon",
    "hunger_summon",
    "scamp_summon",
    "skeleton_summon",
    "ancestor_ghost_variner",
    "fabricant_summon",
    "bm_bear_black_summon",
    "bm_wolf_grey_summon",
    "bm_wolf_bone_summon",
    "centurion_fire_dead",
    "wraith_sul_senipul",
};

bool summon_fixer_t::is_known_summon(const std::string & record_id)
{
	const auto lower_id = string_utils::to_lower(record_id);

	for (const auto & summon_id : known_summon_ids)
	{
		if (lower_id == summon_id)
			return true;
	}

	return false;
}

std::string summon_fixer_t::apply(const std::string & record_id, const std::string & content)
{
	if (!is_known_summon(record_id))
		return {};

	if (content.size() < flags_offset + 4)
		return {};

	const auto flags_byte = static_cast<uint8_t>(content[flags_offset + 1]);
	if ((flags_byte & persistent_flag_byte) != 0)
		return {};

	auto fixed = content;
	fixed[flags_offset + 1] = static_cast<char>(flags_byte | persistent_flag_byte);
	return fixed;
}
