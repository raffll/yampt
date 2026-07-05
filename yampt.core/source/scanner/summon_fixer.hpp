#pragma once

#include <string>

class summon_fixer_t
{
public:
	static std::string apply(const std::string & record_id, const std::string & content);

private:
	static bool is_known_summon(const std::string & record_id);
};
