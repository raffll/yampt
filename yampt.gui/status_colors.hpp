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
	if (status == "in_progress")
		return ImVec4(0.4f, 0.6f, 0.95f, 1.0f);
	return ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
}

inline const char * get_status_display_name(const std::string & status)
{
	if (status == "untranslated")
		return "Untranslated";
	if (status == "missing")
		return "Missing";
	if (status == "duplicate")
		return "Duplicate";
	if (status == "coords")
		return "Coords";
	if (status == "fingerprint")
		return "Fingerprint";
	if (status == "heuristic")
		return "Heuristic";
	if (status == "info")
		return "Info";
	if (status == "exact")
		return "Exact";
	if (status == "wilderness")
		return "Wilderness";
	if (status == "region")
		return "Region";
	if (status == "matched")
		return "Matched";
	if (status == "error")
		return "Error";
	if (status == "identical")
		return "Identical";
	if (status == "translated")
		return "Translated";
	if (status == "reused")
		return "Reused";
	if (status == "adapted")
		return "Adapted";
	if (status == "changed")
		return "Changed";
	if (status == "in_progress")
		return "In Progress";
	if (status == "mismatch")
		return "Mismatch";
	if (status.empty())
		return "(none)";
	return status.c_str();
}

inline const char * get_type_display_name(tools_t::rec_type_t type)
{
	switch (type)
	{
	case tools_t::rec_type_t::cell:
		return "Cells";
	case tools_t::rec_type_t::dial:
		return "Topics";
	case tools_t::rec_type_t::info:
		return "Dialogues";
	case tools_t::rec_type_t::fnam:
		return "Names";
	case tools_t::rec_type_t::text:
		return "Books";
	case tools_t::rec_type_t::gmst:
		return "Settings";
	case tools_t::rec_type_t::desc:
		return "Descriptions";
	case tools_t::rec_type_t::rnam:
		return "Factions";
	case tools_t::rec_type_t::indx:
		return "Index";
	case tools_t::rec_type_t::bnam:
		return "Scripts (BNAM)";
	case tools_t::rec_type_t::sctx:
		return "Scripts (SCTX)";
	default:
		return "Unknown";
	}
}
