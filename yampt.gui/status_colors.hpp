#pragma once

#include <imgui.h>
#include <string>

inline ImVec4 get_status_color(const std::string & status)
{
	if (status == "untranslated")
		return ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
	if (status == "missing")
		return ImVec4(0.95f, 0.55f, 0.35f, 1.0f);
	if (status == "duplicate")
		return ImVec4(0.95f, 0.9f, 0.4f, 1.0f);
	if (status == "coords")
		return ImVec4(0.4f, 0.8f, 0.8f, 1.0f);
	if (status == "fingerprint")
		return ImVec4(0.4f, 0.75f, 0.85f, 1.0f);
	if (status == "heuristic")
		return ImVec4(0.6f, 0.7f, 0.9f, 1.0f);
	if (status == "info")
		return ImVec4(0.45f, 0.85f, 0.75f, 1.0f);
	if (status == "exact")
		return ImVec4(0.5f, 0.9f, 0.85f, 1.0f);
	if (status == "wilderness")
		return ImVec4(0.3f, 0.6f, 0.3f, 1.0f);
	if (status == "region")
		return ImVec4(0.6f, 0.6f, 0.35f, 1.0f);
	if (status == "matched")
		return ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
	if (status == "error")
		return ImVec4(0.95f, 0.4f, 0.4f, 1.0f);
	if (status == "identical")
		return ImVec4(0.55f, 0.75f, 0.55f, 1.0f);
	if (status == "translated")
		return ImVec4(0.5f, 0.9f, 0.5f, 1.0f);
	if (status == "reused")
		return ImVec4(0.5f, 0.85f, 0.7f, 1.0f);
	if (status == "adapted")
		return ImVec4(0.7f, 0.55f, 0.85f, 1.0f);
	if (status == "changed")
		return ImVec4(0.95f, 0.7f, 0.4f, 1.0f);
	return ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
}
