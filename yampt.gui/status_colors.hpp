#pragma once

#include <imgui.h>
#include <string>

inline ImVec4 get_status_color(const std::string & status)
{
	if (status == "translated")
		return ImVec4(0.7f, 0.9f, 0.7f, 1.0f);
	if (status == "auto_identical")
		return ImVec4(0.7f, 0.85f, 0.95f, 1.0f);
	if (status == "auto_heuristic")
		return ImVec4(0.8f, 0.75f, 0.95f, 1.0f);
	if (status == "validated")
		return ImVec4(0.6f, 0.95f, 0.6f, 1.0f);
	if (status == "changed")
		return ImVec4(0.95f, 0.85f, 0.6f, 1.0f);
	if (status == "has_errors")
		return ImVec4(0.95f, 0.7f, 0.7f, 1.0f);
	return ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
}
