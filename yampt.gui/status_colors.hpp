#pragma once

#include <imgui.h>
#include <string>

inline ImVec4 get_status_color(const std::string & status)
{
	if (status.empty() || status == "untranslated")
		return ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
	if (status == "missing")
		return ImVec4(0.95f, 0.55f, 0.35f, 1.0f);
	if (status == "duplicate")
		return ImVec4(0.95f, 0.9f, 0.4f, 1.0f);
	if (status == "matched_by_coords")
		return ImVec4(0.4f, 0.8f, 0.8f, 1.0f);
	if (status == "matched_by_fingerprint")
		return ImVec4(0.4f, 0.75f, 0.85f, 1.0f);
	if (status == "matched_by_heuristic")
		return ImVec4(0.6f, 0.7f, 0.9f, 1.0f);
	if (status == "matched_by_info")
		return ImVec4(0.45f, 0.85f, 0.75f, 1.0f);
	if (status == "matched_by_name")
		return ImVec4(0.5f, 0.9f, 0.85f, 1.0f);
	if (status == "wilderness")
		return ImVec4(0.3f, 0.6f, 0.3f, 1.0f);
	if (status == "region")
		return ImVec4(0.6f, 0.6f, 0.35f, 1.0f);
	if (status == "auto_identical")
		return ImVec4(0.55f, 0.75f, 0.55f, 1.0f);
	if (status == "auto_base")
		return ImVec4(0.5f, 0.9f, 0.5f, 1.0f);
	if (status == "auto_translated")
		return ImVec4(0.5f, 0.85f, 0.7f, 1.0f);
	if (status == "auto_heuristic")
		return ImVec4(0.7f, 0.55f, 0.85f, 1.0f);
	if (status == "auto_changed")
		return ImVec4(0.95f, 0.7f, 0.4f, 1.0f);
	if (status == "in_progress")
		return ImVec4(0.5f, 0.7f, 0.95f, 1.0f);
	if (status == "translated")
		return ImVec4(0.5f, 0.95f, 0.5f, 1.0f);
	if (status == "has_errors")
		return ImVec4(0.95f, 0.4f, 0.4f, 1.0f);
	return ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
}
