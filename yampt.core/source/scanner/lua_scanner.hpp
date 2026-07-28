#pragma once

#include "conflict_detector.hpp"
#include "handler_parser.hpp"
#include "omwscripts_parser.hpp"

#include <atomic>
#include <string>
#include <vector>

struct lua_scan_result_t
{
	std::vector<handler_registration_t> registrations;
	std::vector<handler_conflict_t> conflicts;
	std::vector<std::string> warnings;
};

class lua_scanner_t
{
public:
	lua_scan_result_t scan(const std::vector<std::string> & data_paths,
	                       const std::vector<std::string> & mod_names);
	void cancel();
	bool is_cancelled() const;

private:
	std::atomic<bool> m_cancelled { false };
};
