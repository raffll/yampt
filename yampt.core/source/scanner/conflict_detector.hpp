#pragma once

#include "handler_parser.hpp"
#include <string>
#include <vector>

enum class conflict_severity_t
{
	blocking,
	mutating,
	overlapping
};

struct handler_conflict_t
{
	std::string interface_name;
	std::string method_name;
	std::string type_argument;
	conflict_severity_t severity = conflict_severity_t::overlapping;
	std::vector<handler_registration_t> registrations;
};

class conflict_detector_t
{
public:
	std::vector<handler_conflict_t> detect(const std::vector<handler_registration_t> & all_registrations);
};
