#define NOMINMAX

#include "editor_app.hpp"
#include "encoding_utils.hpp"
#include "imgui.h"
#include "status_colors.hpp"
#include "../yampt/dict_merger.hpp"

#include <algorithm>
#include <array>
#include <cctype>

#include <Windows.h>
#include <commdlg.h>

static constexpr size_t EDIT_BUFFER_SIZE = 8192;

std::string editor_app_t::get_exe_directory() const
{
	char buf[MAX_PATH] = {};
	GetModuleFileNameA(nullptr, buf, MAX_PATH);
	std::string path(buf);
	auto pos = path.find_last_of("\\/");
	if (pos == std::string::npos)
		return ".";
	return path.substr(0, pos);
}

void editor_app_t::init(SDL_Window * window)
{
	window_ = window;
	config_path_ = get_exe_directory() + "\\yampt_gui.ini";
	config_.load(config_path_);
	split_ratio_ = config_.split_ratio;

	sidebar_width_ = config_.sidebar_width;
	bottom_height_ = config_.bottom_height;
	sidebar_visible_ = config_.sidebar_visible;
	bottom_visible_ = config_.bottom_visible;
	encoding_index_ = config_.encoding_index;
	selected_dict_ = config_.selected_dict;

	if (!config_.base_dict_paths.empty())
	{
		reload_base_dicts();
		annotations_mgr_.rebuild(state_, merged_base_dict_);
	}

	if (!config_.last_user_dict_path.empty())
	{
		if (state_.load_user_dict(config_.last_user_dict_path))
		{
			decode_dict_from_codepage(state_.get_user_dict(), active_codepage());
			annotations_mgr_.rebuild(state_, merged_base_dict_);
		}
	}

	if (!config_.spell_check_aff.empty() && !config_.spell_check_dic.empty())
		spell_checker_.load(config_.spell_check_aff, config_.spell_check_dic);

	rebuild_row_data();
}

void editor_app_t::frame()
{
	std::string title = "yampt.gui";
	if (state_.has_unsaved_changes())
		title += " *";
	SDL_SetWindowTitle(window_, title.c_str());

	const ImGuiViewport * viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
	                                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
	                                ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
	                                ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar |
	                                ImGuiWindowFlags_NoScrollWithMouse;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
	ImGui::Begin("##MainWindow", nullptr, window_flags);
	ImGui::PopStyleVar(3);

	render_menu_bar();
	render_toolbar();
	render_status_summary_bar();

	auto available = ImGui::GetContentRegionAvail();
	float status_bar_height = 25.0f;
	float panel_area_height = available.y - status_bar_height;

	int window_w = 0;
	int window_h = 0;
	SDL_GetWindowSize(window_, &window_w, &window_h);
	float window_width = static_cast<float>(window_w);

	sidebar_width_ = clamp_sidebar_width(sidebar_width_, window_width);
	if (bottom_visible_)
		bottom_height_ = clamp_bottom_height(bottom_height_, panel_area_height);

	ImGui::BeginChild(
	    "PanelArea", ImVec2(0, panel_area_height), ImGuiChildFlags_None,
	    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	if (sidebar_visible_)
	{
		render_sidebar();

		float max_w = clamp_sidebar_width(window_width, window_width);
		render_splitter_vertical(sidebar_width_, 150.0f, max_w);
	}

	ImGui::BeginGroup();

	float main_panel_height = ImGui::GetContentRegionAvail().y;
	if (bottom_visible_)
	{
		float splitter_height = 6.0f;
		main_panel_height -= (bottom_height_ + splitter_height);
		if (main_panel_height < 100.0f)
			main_panel_height = 100.0f;
	}

	ImGui::BeginChild(
	    "MainPanelArea", ImVec2(0, main_panel_height), ImGuiChildFlags_None,
	    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	render_main_panel();
	ImGui::EndChild();

	if (bottom_visible_)
	{
		float max_h = clamp_bottom_height(panel_area_height, panel_area_height);
		render_splitter_horizontal(bottom_height_, 100.0f, max_h);
		render_bottom_panel();
	}

	ImGui::EndGroup();

	ImGui::EndChild();

	render_status_bar();
	render_dialogs();

	ImGui::End();
}

void editor_app_t::shutdown()
{
	config_.split_ratio = split_ratio_;
	config_.sidebar_width = sidebar_width_;
	config_.bottom_height = bottom_height_;
	config_.sidebar_visible = sidebar_visible_;
	config_.bottom_visible = bottom_visible_;
	config_.encoding_index = encoding_index_;
	config_.selected_dict = selected_dict_;
	config_.last_user_dict_path = state_.get_user_path();
	config_.last_source_dict_path = state_.get_source_path();
	config_.save(config_path_);
}

bool editor_app_t::wants_quit() const
{
	return quit_requested_;
}

void editor_app_t::request_quit()
{
	if (state_.has_unsaved_changes())
		show_quit_dialog_ = true;
	else
		quit_requested_ = true;
}

std::string editor_app_t::make_lookup_key(tools_t::rec_type_t type, const std::string & key)
{
	return std::to_string(static_cast<int>(type)) + "|" + key;
}

void editor_app_t::rebuild_row_data()
{
	left_rows_.clear();
	right_rows_.clear();
	left_lookup_.clear();
	right_lookup_.clear();

	const auto & active_dict = (selected_dict_ == selected_dict_t::base)    ? merged_base_dict_
	                           : (selected_dict_ == selected_dict_t::source) ? state_.get_source_dict()
	                                                                         : state_.get_user_dict();

	int idx = 0;
	for (const auto & [type, chapter] : active_dict)
	{
		if (type_filter_.count(type) == 0)
			continue;

		for (size_t i = 0; i < chapter.records.size(); ++i)
		{
			if (!status_filter_.empty())
			{
				const auto & s = chapter.records[i].status;
				bool match = s.empty() ? status_filter_.count("untranslated") > 0
				                       : status_filter_.count(s) > 0;
				if (!match)
					continue;
			}

			left_rows_.push_back({ type, i });
			left_lookup_[make_lookup_key(type, chapter.records[i].key_text)] = idx;
			++idx;
		}
	}

	idx = 0;
	for (const auto & [type, chapter] : state_.get_source_dict())
	{
		if (type_filter_.count(type) == 0)
			continue;

		for (size_t i = 0; i < chapter.records.size(); ++i)
		{
			if (!status_filter_.empty())
			{
				const auto & s = chapter.records[i].status;
				bool match = s.empty() ? status_filter_.count("untranslated") > 0
				                       : status_filter_.count(s) > 0;
				if (!match)
					continue;
			}

			right_rows_.push_back({ type, i });
			right_lookup_[make_lookup_key(type, chapter.records[i].key_text)] = idx;
			++idx;
		}
	}
}

void editor_app_t::render_menu_bar()
{
	if (!ImGui::BeginMenuBar())
		return;

	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Open User Dict...", "Ctrl+O"))
		{
			auto path = show_open_file_dialog("Open User Dictionary");
			if (!path.empty())
			{
				state_.load_user_dict(path);
				decode_dict_from_codepage(state_.get_user_dict(), active_codepage());
				annotations_mgr_.rebuild(state_, merged_base_dict_);
				selected_row_left_ = -1;
				selected_row_right_ = -1;
				editing_row_ = -1;
				editing_type_ = tools_t::rec_type_t::unknown;
				edit_focus_pending_ = false;
				rebuild_row_data();
			}
		}

		if (ImGui::MenuItem("Open Source Dict..."))
		{
			auto path = show_open_file_dialog("Open Source Dictionary");
			if (!path.empty())
			{
				state_.load_source_dict(path);
				decode_dict_from_codepage(state_.get_source_dict(), active_codepage());
				annotations_mgr_.rebuild(state_, merged_base_dict_);
				selected_row_left_ = -1;
				selected_row_right_ = -1;
				rebuild_row_data();
			}
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Save", "Ctrl+S"))
		{
			if (state_.get_user_path().empty())
			{
				auto path = show_save_file_dialog("Save User Dictionary");
				if (!path.empty())
					save_user_dict_as_encoded(path);
			}
			else
			{
				save_user_dict_encoded();
			}
		}

		if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
		{
			auto path = show_save_file_dialog("Save User Dictionary As");
			if (!path.empty())
				save_user_dict_as_encoded(path);
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Load NPC Flags..."))
		{
			auto path = show_open_file_dialog("Load NPC Flags Dictionary");
			if (!path.empty())
				annotations_mgr_.load_npc_flags(path);
		}

		if (ImGui::MenuItem("Load Enchantment Dict..."))
		{
			auto path = show_open_file_dialog("Load Enchantment Dictionary");
			if (!path.empty())
				annotations_mgr_.load_enchantments(path);
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Exit", "Alt+F4"))
		{
			request_quit();
		}

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("View"))
	{
		ImGui::MenuItem("Sidebar", nullptr, &sidebar_visible_);
		ImGui::MenuItem("Bottom Panel", nullptr, &bottom_visible_);
		ImGui::Separator();
		ImGui::MenuItem("Annotations", nullptr, &show_annotations_);
		ImGui::MenuItem("History", nullptr, &show_history_);
		ImGui::Separator();
		ImGui::MenuItem("Base Dictionary Config", nullptr, &show_base_dict_config_);
		ImGui::EndMenu();
	}

	ImGui::EndMenuBar();
}

void editor_app_t::render_toolbar()
{
	static constexpr tools_t::rec_type_t filter_types[] = {
		tools_t::rec_type_t::cell, tools_t::rec_type_t::dial, tools_t::rec_type_t::info, tools_t::rec_type_t::fnam,
		tools_t::rec_type_t::text, tools_t::rec_type_t::gmst, tools_t::rec_type_t::desc, tools_t::rec_type_t::rnam,
		tools_t::rec_type_t::indx, tools_t::rec_type_t::bnam, tools_t::rec_type_t::sctx,
	};

	float toolbar_height = 52.0f;
	ImGui::BeginChild("Toolbar", ImVec2(0, toolbar_height), ImGuiChildFlags_None);

	ImGui::SetNextItemWidth(200.0f);
	if (ImGui::InputText("Search", search_buffer_.data(), search_buffer_.size()))
	{
		search_.set_query(search_buffer_.data(), search_case_sensitive_);
		search_.find_all(state_, type_filter_);
	}

	ImGui::SameLine();
	if (ImGui::Checkbox("Case sensitive", &search_case_sensitive_))
	{
		search_.set_query(search_buffer_.data(), search_case_sensitive_);
		search_.find_all(state_, type_filter_);
	}

	if (!search_.get_matches().empty())
	{
		ImGui::SameLine();
		std::string match_info =
		    std::to_string(search_.current_index() + 1) + "/" + std::to_string(search_.get_matches().size());
		ImGui::TextUnformatted(match_info.c_str());
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(220.0f);
	const char * encoding_preview = codepage_name(supported_codepages[encoding_index_]);
	if (ImGui::BeginCombo("Encoding", encoding_preview))
	{
		for (int i = 0; i < static_cast<int>(std::size(supported_codepages)); ++i)
		{
			bool is_selected = (i == encoding_index_);
			if (ImGui::Selectable(codepage_name(supported_codepages[i]), is_selected))
			{
				if (i != encoding_index_)
				{
					codepage_t old_cp = supported_codepages[encoding_index_];
					codepage_t new_cp = supported_codepages[i];
					encoding_index_ = i;
					reencode_dict(state_.get_user_dict(), old_cp, new_cp);
					reencode_dict(state_.get_source_dict(), old_cp, new_cp);
					reencode_dict(merged_base_dict_, old_cp, new_cp);
					annotations_mgr_.rebuild(state_, merged_base_dict_);
					rebuild_row_data();
					config_.encoding_index = encoding_index_;
					config_.save(config_path_);
				}
			}
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	for (size_t i = 0; i < std::size(filter_types); ++i)
	{
		if (i > 0)
			ImGui::SameLine();

		auto type = filter_types[i];
		bool is_active = type_filter_.count(type) > 0;

		size_t count = 0;
		const auto & toolbar_dict = (selected_dict_ == selected_dict_t::base)    ? merged_base_dict_
		                            : (selected_dict_ == selected_dict_t::source) ? state_.get_source_dict()
		                                                                          : state_.get_user_dict();
		auto it = toolbar_dict.find(type);
		if (it != toolbar_dict.end())
			count = it->second.records.size();

		std::string label = tools_t::type_to_str(type) + " (" + std::to_string(count) + ")";

		if (is_active)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.4f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.5f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.1f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.26f, 0.59f, 0.98f, 0.2f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		}

		ImGui::SmallButton(label.c_str());
		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(5);

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			if (type_filter_solo_ && type_filter_.size() == 1 && type_filter_.count(type) > 0)
			{
				type_filter_ = saved_type_filter_;
				type_filter_solo_ = false;
			}
			else
			{
				saved_type_filter_ = type_filter_;
				type_filter_.clear();
				type_filter_.insert(type);
				type_filter_solo_ = true;
			}
			rebuild_row_data();
		}

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			if (is_active)
				type_filter_.erase(type);
			else
				type_filter_.insert(type);
			type_filter_solo_ = false;
			rebuild_row_data();
		}
	}

	bool navigate_next = ImGui::IsKeyPressed(ImGuiKey_F3) && !ImGui::GetIO().KeyShift;
	bool navigate_prev = ImGui::IsKeyPressed(ImGuiKey_F3) && ImGui::GetIO().KeyShift;

	if (navigate_next && !search_.get_matches().empty())
	{
		search_.next_match();
		const auto * match = search_.current_match();
		if (match)
		{
			for (int i = 0; i < static_cast<int>(left_rows_.size()); ++i)
			{
				if (left_rows_[i].type == match->type && left_rows_[i].record_index == match->record_index)
				{
					scroll_to_row_left_ = i;
					selected_row_left_ = i;
					selected_row_ = i;
					break;
				}
			}
		}
	}

	if (navigate_prev && !search_.get_matches().empty())
	{
		search_.prev_match();
		const auto * match = search_.current_match();
		if (match)
		{
			for (int i = 0; i < static_cast<int>(left_rows_.size()); ++i)
			{
				if (left_rows_[i].type == match->type && left_rows_[i].record_index == match->record_index)
				{
					scroll_to_row_left_ = i;
					selected_row_left_ = i;
					selected_row_ = i;
					break;
				}
			}
		}
	}

	if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S))
	{
		if (state_.get_user_path().empty())
		{
			auto path = show_save_file_dialog("Save User Dictionary");
			if (!path.empty())
				save_user_dict_as_encoded(path);
		}
		else
		{
			save_user_dict_encoded();
		}
	}

	ImGui::EndChild();
}

void editor_app_t::render_sidebar()
{
	if (!sidebar_visible_)
		return;

	ImGui::BeginChild("Sidebar", ImVec2(sidebar_width_, 0), ImGuiChildFlags_Borders);

	ImGui::TextUnformatted("Dictionaries");
	ImGui::Separator();

	const auto & user_path = state_.get_user_path();
	{
		std::string label = "User Dict";
		if (!user_path.empty())
		{
			auto pos = user_path.find_last_of("\\/");
			label += ": " + ((pos != std::string::npos) ? user_path.substr(pos + 1) : user_path);
		}
		bool is_selected = (selected_dict_ == selected_dict_t::user);
		if (ImGui::Selectable(label.c_str(), is_selected))
		{
			save_current_filters();
			selected_dict_ = selected_dict_t::user;
			restore_filters_for(selected_dict_t::user);
			selected_row_ = -1;
			selected_row_left_ = -1;
			scroll_to_row_ = -1;
			editing_row_ = -1;
			editing_type_ = tools_t::rec_type_t::unknown;
			edit_focus_pending_ = false;
		}
		if (!user_path.empty() && ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", user_path.c_str());
	}

	if (!config_.base_dict_paths.empty())
	{
		std::string label = "Base Dict (" + std::to_string(config_.base_dict_paths.size()) + " files)";
		bool is_selected = (selected_dict_ == selected_dict_t::base);
		if (ImGui::Selectable(label.c_str(), is_selected))
		{
			save_current_filters();
			selected_dict_ = selected_dict_t::base;
			restore_filters_for(selected_dict_t::base);
			selected_row_ = -1;
			selected_row_left_ = -1;
			scroll_to_row_ = -1;
			editing_row_ = -1;
			editing_type_ = tools_t::rec_type_t::unknown;
			edit_focus_pending_ = false;
			reload_base_dicts();
		}
		if (ImGui::IsItemHovered())
		{
			std::string tip;
			for (const auto & p : config_.base_dict_paths)
			{
				if (!tip.empty())
					tip += "\n";
				tip += p;
			}
			ImGui::SetTooltip("%s", tip.c_str());
		}
	}

	const auto & source_path = state_.get_source_path();
	if (!source_path.empty())
	{
		std::string label = "Source Dict";
		auto pos = source_path.find_last_of("\\/");
		label += ": " + ((pos != std::string::npos) ? source_path.substr(pos + 1) : source_path);
		bool is_selected = (selected_dict_ == selected_dict_t::source);
		if (ImGui::Selectable(label.c_str(), is_selected))
		{
			save_current_filters();
			selected_dict_ = selected_dict_t::source;
			restore_filters_for(selected_dict_t::source);
			selected_row_ = -1;
			selected_row_left_ = -1;
			scroll_to_row_ = -1;
			editing_row_ = -1;
			editing_type_ = tools_t::rec_type_t::unknown;
			edit_focus_pending_ = false;
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", source_path.c_str());
	}

	ImGui::EndChild();
}

void editor_app_t::render_status_summary_bar()
{
	static const char * status_names[] = {
		"untranslated",    "missing",          "duplicate",       "matched_by_coords",
		"matched_by_info", "matched_by_name",  "wilderness",      "region",
		"auto_identical",  "auto_base",        "auto_translated", "auto_heuristic",
		"auto_changed",    "in_progress",      "translated",      "has_errors"
	};
	static const char * status_labels[] = {
		"Untranslated",    "Missing",          "Duplicate",       "Matched Coords",
		"Matched Info",    "Matched Name",     "Wilderness",      "Region",
		"Auto Identical",  "Auto Base",        "Auto Translated", "Auto Heuristic",
		"Auto Changed",    "In Progress",      "Translated",      "Has Errors"
	};
	static constexpr size_t status_count = 16;

	int counts[status_count] = {};

	const auto & active_dict = (selected_dict_ == selected_dict_t::base)    ? merged_base_dict_
	                           : (selected_dict_ == selected_dict_t::source) ? state_.get_source_dict()
	                                                                         : state_.get_user_dict();

	for (const auto & [type, chapter] : active_dict)
	{
		if (type_filter_.count(type) == 0)
			continue;

		for (size_t i = 0; i < chapter.records.size(); ++i)
		{
			const std::string & status = chapter.records[i].status;

			if (status.empty())
			{
				++counts[0];
				continue;
			}

			for (size_t j = 0; j < status_count; ++j)
			{
				if (status == status_names[j])
				{
					++counts[j];
					break;
				}
			}
		}
	}

	ImGui::BeginChild("StatusSummary", ImVec2(0, 25), ImGuiChildFlags_None);

	bool first = true;
	for (size_t i = 0; i < status_count; ++i)
	{
		if (counts[i] == 0)
			continue;

		if (!first)
			ImGui::SameLine();
		first = false;

		bool is_active = status_filter_.empty() || status_filter_.count(status_names[i]) > 0;

		ImVec4 color = get_status_color(status_names[i]);
		if (is_active)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, color);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Border, color);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(color.x, color.y, color.z, 0.2f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(color.x, color.y, color.z, 0.3f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Border, color);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		}

		std::string label = std::string(status_labels[i]) + ": " + std::to_string(counts[i]);
		ImGui::SmallButton(label.c_str());
		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(5);

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			std::string status = status_names[i];
			if (status_filter_solo_ && status_filter_.size() == 1 && status_filter_.count(status) > 0)
			{
				status_filter_ = saved_status_filter_;
				status_filter_solo_ = false;
			}
			else
			{
				saved_status_filter_ = status_filter_;
				status_filter_.clear();
				status_filter_.insert(status);
				status_filter_solo_ = true;
			}
			rebuild_row_data();
		}

		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			std::string status = status_names[i];
			if (status_filter_.empty())
			{
				for (size_t j = 0; j < status_count; ++j)
				{
					if (j != i)
						status_filter_.insert(status_names[j]);
				}
			}
			else if (status_filter_.count(status) > 0)
			{
				status_filter_.erase(status);
				if (status_filter_.empty())
					status_filter_.clear();
			}
			else
			{
				status_filter_.insert(status);
			}
			status_filter_solo_ = false;
			rebuild_row_data();
		}
	}

	if (!first)
		ImGui::SameLine();

	std::string total = "Total: " + std::to_string(static_cast<int>(left_rows_.size()));
	ImGui::TextUnformatted(total.c_str());

	ImGui::EndChild();
}

void editor_app_t::render_panels()
{
	auto available = ImGui::GetContentRegionAvail();
	float splitter_width = 4.0f;
	float left_width = available.x * split_ratio_;
	float right_width = available.x * (1.0f - split_ratio_) - splitter_width;

	ImGui::BeginChild("LeftPanel", ImVec2(left_width, available.y), ImGuiChildFlags_Borders);
	render_dict_table("##left_table", state_.get_user_dict(), &state_.get_user_dict(), true);
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::Button("##splitter", ImVec2(splitter_width, available.y));
	if (ImGui::IsItemActive())
		split_ratio_ += ImGui::GetIO().MouseDelta.x / available.x;
	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	split_ratio_ = std::clamp(split_ratio_, 0.1f, 0.9f);

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.16f, 1.0f));
	ImGui::BeginChild("RightPanel", ImVec2(right_width, available.y), ImGuiChildFlags_Borders);
	render_dict_table("##right_table", state_.get_source_dict(), nullptr, false);
	ImGui::EndChild();
	ImGui::PopStyleColor();
}

void editor_app_t::render_main_panel()
{
	ImGui::BeginChild("MainPanel", ImVec2(0, 0), ImGuiChildFlags_Borders);

	bool is_editable = (selected_dict_ == selected_dict_t::user);

	const auto & active_dict = (selected_dict_ == selected_dict_t::base)    ? merged_base_dict_
	                           : (selected_dict_ == selected_dict_t::source) ? state_.get_source_dict()
	                                                                         : state_.get_user_dict();

	const auto & rows = left_rows_;
	int & selected_row = selected_row_;
	int & scroll_to_row = scroll_to_row_;

	if (!is_editable)
	{
		const char * mode_label = (selected_dict_ == selected_dict_t::base) ? "[Read-Only: Base Dict]"
		                                                                    : "[Read-Only: Source Dict]";
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.3f, 1.0f));
		ImGui::TextUnformatted(mode_label);
		ImGui::PopStyleColor();
	}

	const ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
	                              ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;

	if (!ImGui::BeginTable("##main_table", 5, flags))
	{
		ImGui::EndChild();
		return;
	}

	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, config_.column_widths[0]);
	ImGui::TableSetupColumn("Original", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Translation", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, config_.column_widths[3]);
	ImGui::TableSetupColumn("##validation", ImGuiTableColumnFlags_WidthFixed, 24.0f);
	ImGui::TableHeadersRow();

	float row_height = ImGui::GetTextLineHeightWithSpacing();

	if (scroll_to_row >= 0 && scroll_to_row < static_cast<int>(rows.size()))
	{
		float target_y = scroll_to_row * row_height;
		ImGui::SetScrollY(target_y);
		scroll_to_row = -1;
	}

	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(rows.size()));

	while (clipper.Step())
	{
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
		{
			const auto & row_ref = rows[i];
			auto it = active_dict.find(row_ref.type);
			if (it == active_dict.end())
				continue;
			if (row_ref.record_index >= it->second.records.size())
				continue;

			const auto & entry = it->second.records[row_ref.record_index];

			ImGui::TableNextRow();

			bool is_selected = (i == selected_row);
			if (is_selected)
			{
				ImU32 highlight_color = IM_COL32(50, 80, 130, 100);
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, highlight_color);
			}

			ImGui::TableSetColumnIndex(0);
			if (is_editable && history_.is_modified_this_session(row_ref.type, entry.key_text))
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
				ImGui::TextUnformatted("\xe2\x97\x8f ");
				ImGui::PopStyleColor();
				ImGui::SameLine(0, 0);
			}
			std::string id_text = tools_t::type_to_str(row_ref.type) + ": " + entry.key_text;
			ImGui::PushID(i);

			if (ImGui::Selectable(
			        id_text.c_str(),
			        is_selected,
			        ImGuiSelectableFlags_SpanAllColumns,
			        ImVec2(0, 0)))
			{
				selected_row = i;
				selected_row_left_ = i;
			}

			if (is_editable && ImGui::BeginPopupContextItem("##row_ctx"))
			{
				if (ImGui::BeginMenu("Set Status"))
				{
					static const char * ctx_status_names[] = { "untranslated", "in_progress", "translated", "has_errors" };
					static const char * ctx_status_labels[] = { "Untranslated", "In Progress", "Translated", "Has Errors" };

					for (int s = 0; s < 4; ++s)
					{
						if (ImGui::MenuItem(ctx_status_labels[s]))
						{
							auto mit = state_.get_user_dict().find(row_ref.type);
							if (mit != state_.get_user_dict().end() &&
							    row_ref.record_index < mit->second.records.size())
							{
								auto & rec = mit->second.records[row_ref.record_index];
								std::string old_status = rec.status;
								rec.status = ctx_status_names[s];
								state_.mark_modified(row_ref.type, row_ref.record_index);
								history_.record_change(
								    row_ref.type, entry.key_text, old_status, rec.status);
							}
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}

			ImGui::TableSetColumnIndex(1);
			{
				const char * end = entry.old_text.c_str();
				while (*end && *end != '\r' && *end != '\n')
					++end;
				ImGui::TextUnformatted(entry.old_text.c_str(), end);
			}

			ImGui::TableSetColumnIndex(2);
			{
				const char * end = entry.new_text.c_str();
				while (*end && *end != '\r' && *end != '\n')
					++end;
				ImGui::TextUnformatted(entry.new_text.c_str(), end);
			}

			ImGui::TableSetColumnIndex(3);
			ImVec4 status_color = get_status_color(entry.status);
			ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::ColorConvertFloat4ToU32(status_color));

			std::string status_label = entry.status.empty() ? "(none)" : entry.status;
			if (is_editable)
			{
				if (ImGui::Selectable(status_label.c_str(), false, ImGuiSelectableFlags_None))
					ImGui::OpenPopup("##status_popup");

				if (ImGui::BeginPopup("##status_popup"))
				{
					static const char * popup_status_names[] = { "untranslated", "in_progress", "translated", "has_errors" };
					static const char * popup_status_labels[] = { "Untranslated", "In Progress", "Translated", "Has Errors" };

					for (int s = 0; s < 4; ++s)
					{
						if (ImGui::Selectable(popup_status_labels[s]))
						{
							auto mit = state_.get_user_dict().find(row_ref.type);
							if (mit != state_.get_user_dict().end() &&
							    row_ref.record_index < mit->second.records.size())
							{
								auto & rec = mit->second.records[row_ref.record_index];
								std::string old_status = rec.status;
								rec.status = popup_status_names[s];
								state_.mark_modified(row_ref.type, row_ref.record_index);
								history_.record_change(
								    row_ref.type, entry.key_text, old_status, rec.status);
							}
						}
					}
					ImGui::EndPopup();
				}
			}
			else
			{
				ImGui::TextUnformatted(status_label.c_str());
			}

			ImGui::TableSetColumnIndex(4);
			if (row_ref.type == tools_t::rec_type_t::fnam && annotations_mgr_.has_enchantment(entry.key_text))
			{
				ImGui::TextUnformatted("\xe2\x9a\xa1");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", annotations_mgr_.get_enchantment(entry.key_text).c_str());
			}
			else
			{
				auto result = validation_.validate(row_ref.type, entry.new_text);
				if (result.level == validation_level_t::caution)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
					ImGui::TextUnformatted("\xe2\x9a\xa0");
					ImGui::PopStyleColor();
					if (ImGui::IsItemHovered())
					{
						std::string tip =
						    std::to_string(result.byte_count) + " / " + std::to_string(result.limit) + " bytes";
						ImGui::SetTooltip("%s", tip.c_str());
					}
				}
				else if (result.level == validation_level_t::error)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
					ImGui::TextUnformatted("\xe2\x9b\x94");
					ImGui::PopStyleColor();
					if (ImGui::IsItemHovered())
					{
						std::string tip = std::to_string(result.byte_count) + " / " + std::to_string(result.limit) +
						                  " bytes (OVER LIMIT)";
						ImGui::SetTooltip("%s", tip.c_str());
					}
				}
			}

			ImGui::PopID();
		}
	}

	ImGui::EndTable();
	ImGui::EndChild();
}

void editor_app_t::render_dict_table(
    const char * table_id,
    const tools_t::dict_t & dict,
    tools_t::dict_t * mutable_dict,
    bool is_left_panel)
{
	const auto & rows = is_left_panel ? left_rows_ : right_rows_;
	int & selected_row = is_left_panel ? selected_row_left_ : selected_row_right_;
	int & scroll_to_row = is_left_panel ? scroll_to_row_left_ : scroll_to_row_right_;

	ImGui::Text("Records: %d", static_cast<int>(rows.size()));
	ImGui::Separator();

	const ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
	                              ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;

	if (!ImGui::BeginTable(table_id, 5, flags))
		return;

	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, config_.column_widths[0]);
	ImGui::TableSetupColumn("Original", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Translation", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, config_.column_widths[3]);
	ImGui::TableSetupColumn("##validation", ImGuiTableColumnFlags_WidthFixed, 24.0f);
	ImGui::TableHeadersRow();

	float row_height = ImGui::GetTextLineHeightWithSpacing();

	if (scroll_to_row >= 0 && scroll_to_row < static_cast<int>(rows.size()))
	{
		float target_y = scroll_to_row * row_height;
		ImGui::SetScrollY(target_y);
		scroll_to_row = -1;
	}

	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(rows.size()));

	while (clipper.Step())
	{
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
		{
			const auto & row_ref = rows[i];
			auto it = dict.find(row_ref.type);
			if (it == dict.end())
				continue;
			if (row_ref.record_index >= it->second.records.size())
				continue;

			const auto & entry = it->second.records[row_ref.record_index];

			ImGui::TableNextRow();

			bool is_selected = (i == selected_row);
			if (is_selected)
			{
				ImU32 highlight_color = IM_COL32(50, 80, 130, 100);
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, highlight_color);
			}

			ImGui::TableSetColumnIndex(0);
			if (is_left_panel && history_.is_modified_this_session(row_ref.type, entry.key_text))
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
				ImGui::TextUnformatted("\xe2\x97\x8f ");
				ImGui::PopStyleColor();
				ImGui::SameLine(0, 0);
			}
			std::string id_text = tools_t::type_to_str(row_ref.type) + ": " + entry.key_text;
			ImGui::PushID(i);

			if (ImGui::Selectable(
			        id_text.c_str(),
			        is_selected,
			        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick,
			        ImVec2(0, 0)))
			{
				selected_row = i;
				if (is_left_panel)
					selected_row_ = i;

				std::string lk = make_lookup_key(row_ref.type, entry.key_text);
				if (is_left_panel)
				{
					auto found = right_lookup_.find(lk);
					if (found != right_lookup_.end())
					{
						scroll_to_row_right_ = found->second;
						selected_row_right_ = found->second;
					}
				}
				else
				{
					auto found = left_lookup_.find(lk);
					if (found != left_lookup_.end())
					{
						scroll_to_row_left_ = found->second;
						selected_row_left_ = found->second;
						selected_row_ = found->second;
					}
				}
			}

			bool double_clicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

			if (is_left_panel && mutable_dict)
			{
				std::string ctx_id = "##ctx_" + std::to_string(i);
				if (ImGui::BeginPopupContextItem(ctx_id.c_str()))
				{
					auto mit = mutable_dict->find(row_ref.type);
					if (mit == mutable_dict->end() || row_ref.record_index >= mit->second.records.size())
					{
						ImGui::EndPopup();
					}
					else
					{
						auto & rec = mit->second.records[row_ref.record_index];

						if (ImGui::MenuItem("Set Untranslated"))
						{
							rec.status = tools_t::status_t::untranslated;
							state_.mark_modified(row_ref.type, row_ref.record_index);
						}
						if (ImGui::MenuItem("Set In Progress"))
						{
							rec.status = tools_t::status_t::in_progress;
							state_.mark_modified(row_ref.type, row_ref.record_index);
						}
						if (ImGui::MenuItem("Set Translated"))
						{
							rec.status = tools_t::status_t::translated;
							state_.mark_modified(row_ref.type, row_ref.record_index);
						}

						ImGui::EndPopup();
					}
				}
			}

			ImGui::TableSetColumnIndex(1);
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
			if (search_.get_matches().empty())
			{
				if (row_ref.type == tools_t::rec_type_t::sctx || row_ref.type == tools_t::rec_type_t::text)
					render_text_with_syntax(entry.old_text, row_ref.type);
				else if (row_ref.type == tools_t::rec_type_t::info)
					render_text_with_topic_highlights(entry.old_text);
				else
					ImGui::TextUnformatted(entry.old_text.c_str());
			}
			else
				render_text_with_highlights(entry.old_text, row_ref.type, row_ref.record_index, true);
			ImGui::PopTextWrapPos();

			ImGui::TableSetColumnIndex(2);

			bool is_editing = false;

			if (is_editing)
			{
				ImGui::PushItemWidth(-1);
				bool commit = false;
				bool cancel = false;

				if (edit_multiline_)
				{
					ImGui::InputTextMultiline(
					    "##edit",
					    edit_buffer_.data(),
					    edit_buffer_.size(),
					    ImVec2(-1, row_height * 4),
					    ImGuiInputTextFlags_AllowTabInput);

					if (ImGui::IsKeyPressed(ImGuiKey_Enter) && ImGui::GetIO().KeyCtrl)
						commit = true;
					if (ImGui::IsKeyPressed(ImGuiKey_Escape))
						cancel = true;
				}
				else
				{
					if (ImGui::InputText(
					        "##edit", edit_buffer_.data(), edit_buffer_.size(), ImGuiInputTextFlags_EnterReturnsTrue))
						commit = true;
					if (ImGui::IsKeyPressed(ImGuiKey_Escape))
						cancel = true;
				}

				if (commit)
				{
					std::string new_value(edit_buffer_.data());
					auto mit = mutable_dict->find(row_ref.type);
					if (mit != mutable_dict->end() && row_ref.record_index < mit->second.records.size())
					{
						auto & rec = mit->second.records[row_ref.record_index];
						rec.new_text = new_value;
						rec.status = tools_t::status_t::in_progress;
						auto vr = validation_.validate(row_ref.type, new_value);
						if (vr.level == validation_level_t::error)
							rec.status = tools_t::status_t::has_errors;
						state_.mark_modified(row_ref.type, row_ref.record_index);
						if (row_ref.type == tools_t::rec_type_t::dial)
							annotations_mgr_.rebuild(state_, merged_base_dict_);
					}
					editing_row_ = -1;
					editing_type_ = tools_t::rec_type_t::unknown;
				}

				if (cancel)
				{
					editing_row_ = -1;
					editing_type_ = tools_t::rec_type_t::unknown;
				}

				ImGui::PopItemWidth();
			}
			else
			{
				ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
				if (search_.get_matches().empty())
				{
					if (row_ref.type == tools_t::rec_type_t::sctx || row_ref.type == tools_t::rec_type_t::text)
						render_text_with_syntax(entry.new_text, row_ref.type);
					else if (row_ref.type == tools_t::rec_type_t::info)
						render_text_with_topic_highlights(entry.new_text);
					else
						ImGui::TextUnformatted(entry.new_text.c_str());
				}
				else
					render_text_with_highlights(entry.new_text, row_ref.type, row_ref.record_index, false);
				ImGui::PopTextWrapPos();
			}

			ImGui::TableSetColumnIndex(3);
			ImVec4 status_color = get_status_color(entry.status);
			ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::ColorConvertFloat4ToU32(status_color));
			ImGui::TextUnformatted(entry.status.c_str());

			ImGui::TableSetColumnIndex(4);
			if (row_ref.type == tools_t::rec_type_t::fnam && annotations_mgr_.has_enchantment(entry.key_text))
			{
				ImGui::TextUnformatted("\xe2\x9a\xa1");
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("%s", annotations_mgr_.get_enchantment(entry.key_text).c_str());
			}
			else
			{
				auto result = validation_.validate(row_ref.type, entry.new_text);
				if (result.level == validation_level_t::caution)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
					ImGui::TextUnformatted("\xe2\x9a\xa0");
					ImGui::PopStyleColor();
					if (ImGui::IsItemHovered())
					{
						std::string tip =
						    std::to_string(result.byte_count) + " / " + std::to_string(result.limit) + " bytes";
						ImGui::SetTooltip("%s", tip.c_str());
					}
				}
				else if (result.level == validation_level_t::error)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
					ImGui::TextUnformatted("\xe2\x9b\x94");
					ImGui::PopStyleColor();
					if (ImGui::IsItemHovered())
					{
						std::string tip = std::to_string(result.byte_count) + " / " + std::to_string(result.limit) +
						                  " bytes (OVER LIMIT)";
						ImGui::SetTooltip("%s", tip.c_str());
					}
				}
			}

			ImGui::PopID();
		}
	}

	if (is_left_panel) {}

	ImGui::EndTable();
}

void editor_app_t::render_text_with_highlights(
    const std::string & text,
    tools_t::rec_type_t type,
    size_t record_index,
    bool is_key)
{
	const auto & matches = search_.get_matches();

	std::vector<const search_match_t *> relevant;
	for (const auto & m : matches)
	{
		if (m.type != type)
			continue;
		if (m.record_index != record_index)
			continue;
		if (m.in_key != is_key)
			continue;
		relevant.push_back(&m);
	}

	if (relevant.empty())
	{
		ImGui::TextUnformatted(text.c_str());
		return;
	}

	std::sort(
	    relevant.begin(),
	    relevant.end(),
	    [](const search_match_t * a, const search_match_t * b) { return a->char_start < b->char_start; });

	ImVec2 start_pos = ImGui::GetCursorScreenPos();
	float wrap_width = ImGui::GetContentRegionAvail().x;

	ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrap_width);
	ImGui::TextUnformatted(text.c_str());
	ImGui::PopTextWrapPos();

	ImDrawList * draw_list = ImGui::GetWindowDrawList();
	ImVec4 highlight_color(1.0f, 0.9f, 0.2f, 0.4f);
	ImU32 highlight_u32 = ImGui::ColorConvertFloat4ToU32(highlight_color);
	ImFont * font = ImGui::GetFont();
	float font_size = ImGui::GetFontSize();

	for (const auto * m : relevant)
	{
		size_t start = m->char_start;
		size_t end = m->char_end;

		if (start > text.size())
			start = text.size();
		if (end > text.size())
			end = text.size();
		if (end <= start)
			continue;

		const char * text_start = text.c_str();

		float line_x = start_pos.x;
		float line_y = start_pos.y;
		const char * line_start = text_start;

		for (const char * p = text_start; p < text_start + start;)
		{
			if (*p == '\n')
			{
				line_y += ImGui::GetTextLineHeightWithSpacing();
				line_x = start_pos.x;
				++p;
				line_start = p;
				continue;
			}

			float char_width = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, p, p + 1).x;
			if (line_x + char_width > start_pos.x + wrap_width && p > line_start)
			{
				line_y += ImGui::GetTextLineHeightWithSpacing();
				line_x = start_pos.x;
				line_start = p;
			}
			line_x += char_width;
			++p;
		}

		float rect_x = line_x;
		float rect_y = line_y;

		for (const char * p = text_start + start; p < text_start + end;)
		{
			if (*p == '\n')
			{
				if (line_x > rect_x)
					draw_list->AddRectFilled(
					    ImVec2(rect_x, rect_y),
					    ImVec2(line_x, rect_y + ImGui::GetTextLineHeight()),
					    highlight_u32);
				line_y += ImGui::GetTextLineHeightWithSpacing();
				line_x = start_pos.x;
				rect_x = start_pos.x;
				rect_y = line_y;
				++p;
				line_start = p;
				continue;
			}

			float char_width = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, p, p + 1).x;
			if (line_x + char_width > start_pos.x + wrap_width && p > line_start)
			{
				draw_list->AddRectFilled(
				    ImVec2(rect_x, rect_y),
				    ImVec2(line_x, rect_y + ImGui::GetTextLineHeight()),
				    highlight_u32);
				line_y += ImGui::GetTextLineHeightWithSpacing();
				line_x = start_pos.x;
				rect_x = start_pos.x;
				rect_y = line_y;
				line_start = p;
			}
			line_x += char_width;
			++p;
		}

		draw_list->AddRectFilled(
		    ImVec2(rect_x, rect_y),
		    ImVec2(line_x, rect_y + ImGui::GetTextLineHeight()),
		    highlight_u32);
	}
}

void editor_app_t::render_text_with_syntax(const std::string & text, tools_t::rec_type_t type)
{
	auto tokens = syntax_.tokenize(text, type);

	if (tokens.empty())
	{
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
		ImGui::TextUnformatted(text.c_str());
		ImGui::PopTextWrapPos();
		return;
	}

	ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);

	bool first_segment = true;

	for (const auto & token : tokens)
	{
		if (token.start >= token.end || token.start >= text.size())
			continue;

		size_t end = std::min(token.end, text.size());

		if (!first_segment)
			ImGui::SameLine(0, 0);

		switch (token.type)
		{
		case token_type_t::mwscript_function:
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
			ImGui::TextUnformatted(text.c_str() + token.start, text.c_str() + end);
			ImGui::PopStyleColor();
			break;
		case token_type_t::mwscript_comment:
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
			ImGui::TextUnformatted(text.c_str() + token.start, text.c_str() + end);
			ImGui::PopStyleColor();
			break;
		case token_type_t::mwscript_string:
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
			ImGui::TextUnformatted(text.c_str() + token.start, text.c_str() + end);
			ImGui::PopStyleColor();
			break;
		case token_type_t::html_tag:
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.6f, 1.0f));
			ImGui::TextUnformatted(text.c_str() + token.start, text.c_str() + end);
			ImGui::PopStyleColor();
			break;
		default:
			ImGui::TextUnformatted(text.c_str() + token.start, text.c_str() + end);
			break;
		}

		first_segment = false;
	}

	ImGui::PopTextWrapPos();
}

void editor_app_t::render_text_with_topic_highlights(const std::string & text)
{
	auto annotations = annotations_mgr_.annotate(text, tools_t::rec_type_t::info);

	if (annotations.empty())
	{
		ImGui::TextWrapped("%s", text.c_str());
		return;
	}

	ImVec2 start_pos = ImGui::GetCursorScreenPos();
	float wrap_width = ImGui::GetContentRegionAvail().x;

	ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrap_width);
	ImGui::TextUnformatted(text.c_str());
	ImGui::PopTextWrapPos();

	ImDrawList * draw_list = ImGui::GetWindowDrawList();
	ImU32 highlight_color = IM_COL32(70, 130, 200, 60);
	ImFont * font = ImGui::GetFont();
	float font_size = ImGui::GetFontSize();

	for (const auto & ann : annotations)
	{
		if (ann.start >= text.size() || ann.end > text.size())
			continue;

		const char * text_start = text.c_str();
		const char * text_end = text.c_str() + text.size();
		const char * word_start = text.c_str() + ann.start;
		const char * word_end = text.c_str() + ann.end;

		float line_x = start_pos.x;
		float line_y = start_pos.y;
		const char * line_start = text_start;
		const char * s = text_start;

		while (s < word_start)
		{
			const char * line_end = font->CalcWordWrapPositionA(font_size / font->FontSize, s, text_end, wrap_width);
			if (line_end == s)
				++line_end;

			if (word_start >= s && word_start < line_end)
				break;

			line_start = line_end;
			while (line_start < text_end && (*line_start == ' ' || *line_start == '\n'))
				++line_start;
			s = line_start;
			line_y += font_size;
		}

		float word_offset_x = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, s, word_start).x;
		float word_width = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, word_start, word_end).x;

		ImVec2 rect_min(start_pos.x + word_offset_x, line_y);
		ImVec2 rect_max(start_pos.x + word_offset_x + word_width, line_y + font_size);

		draw_list->AddRectFilled(rect_min, rect_max, highlight_color);
	}
}

void editor_app_t::render_translation_with_spellcheck(const std::string & text, int row_index)
{
	if (!spell_checker_.is_loaded() || text.empty())
	{
		ImGui::TextUnformatted(text.c_str());
		return;
	}

	auto misspelled = spell_checker_.find_misspelled(text);

	if (misspelled.empty())
	{
		ImGui::TextUnformatted(text.c_str());
		return;
	}

	ImDrawList * draw_list = ImGui::GetWindowDrawList();
	ImU32 underline_color = IM_COL32(220, 50, 50, 220);
	float wave_height = 2.0f;
	float wave_period = 4.0f;

	size_t pos = 0;
	bool first_segment = true;

	for (const auto & match : misspelled)
	{
		if (match.start < pos)
			continue;

		if (match.start > pos)
		{
			if (!first_segment)
				ImGui::SameLine(0, 0);
			ImGui::TextUnformatted(text.c_str() + pos, text.c_str() + match.start);
			first_segment = false;
		}

		if (!first_segment)
			ImGui::SameLine(0, 0);

		ImVec2 cursor = ImGui::GetCursorScreenPos();
		ImVec2 text_size = ImGui::CalcTextSize(text.c_str() + match.start, text.c_str() + match.end);

		ImGui::TextUnformatted(text.c_str() + match.start, text.c_str() + match.end);

		float base_y = cursor.y + text_size.y - 1.0f;
		float x = cursor.x;
		float end_x = cursor.x + text_size.x;

		while (x < end_x)
		{
			float next_x = x + wave_period;
			if (next_x > end_x)
				next_x = end_x;

			float mid_x = (x + next_x) * 0.5f;
			float y_offset = ((static_cast<int>((x - cursor.x) / wave_period)) % 2 == 0) ? -wave_height : wave_height;

			draw_list->AddBezierQuadratic(
			    ImVec2(x, base_y),
			    ImVec2(mid_x, base_y + y_offset),
			    ImVec2(next_x, base_y),
			    underline_color,
			    1.5f);

			x = next_x;
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			spell_ctx_row_ = row_index;
			spell_ctx_word_ = match.word;
			spell_ctx_start_ = match.start;
			spell_ctx_end_ = match.end;
			ImGui::OpenPopup("##spell_popup");
		}

		first_segment = false;
		pos = match.end;
	}

	if (pos < text.size())
	{
		if (!first_segment)
			ImGui::SameLine(0, 0);
		ImGui::TextUnformatted(text.c_str() + pos, text.c_str() + text.size());
	}
}

void editor_app_t::render_status_bar()
{
	ImGui::BeginChild("StatusBar", ImVec2(0, 25), ImGuiChildFlags_None);

	std::string info = "Records: " + std::to_string(left_rows_.size());
	ImGui::TextUnformatted(info.c_str());

	ImGui::EndChild();
}

void editor_app_t::render_bottom_panel()
{
	if (!bottom_visible_)
		return;

	ImGui::BeginChild("BottomPanel", ImVec2(0, bottom_height_), ImGuiChildFlags_Borders);

	if (!ImGui::BeginTabBar("BottomTabs"))
	{
		ImGui::EndChild();
		return;
	}

	if (ImGui::BeginTabItem("Editor"))
	{
		active_bottom_tab_ = 0;
		if (selected_row_ < 0 || selected_row_ >= static_cast<int>(left_rows_.size()))
			ImGui::TextDisabled("No record selected");
		else
			render_editor_tab();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Annotations"))
	{
		active_bottom_tab_ = 1;
		if (selected_row_ < 0 || selected_row_ >= static_cast<int>(left_rows_.size()))
			ImGui::TextDisabled("No record selected");
		else
			render_annotations_tab();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("History"))
	{
		active_bottom_tab_ = 2;
		if (selected_row_ < 0 || selected_row_ >= static_cast<int>(left_rows_.size()))
			ImGui::TextDisabled("No record selected");
		else
			render_history_tab();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Speaker"))
	{
		active_bottom_tab_ = 3;
		render_speaker_tab();
		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();
	ImGui::EndChild();
}

void editor_app_t::render_editor_tab()
{
	const auto & row = left_rows_[selected_row_];

	const auto & active_dict = (selected_dict_ == selected_dict_t::base)    ? merged_base_dict_
	                           : (selected_dict_ == selected_dict_t::source) ? state_.get_source_dict()
	                                                                         : state_.get_user_dict();
	bool is_editable = (selected_dict_ == selected_dict_t::user);

	auto it = active_dict.find(row.type);
	if (it == active_dict.end() || row.record_index >= it->second.records.size())
		return;

	const auto & entry = it->second.records[row.record_index];
	float available_width = ImGui::GetContentRegionAvail().x;
	float splitter_width = 6.0f;
	float left_width = available_width * split_ratio_ - splitter_width * 0.5f;
	float right_width = available_width - left_width - splitter_width;
	float panel_height = ImGui::GetContentRegionAvail().y;

	ImGui::BeginChild("##editor_original", ImVec2(left_width, panel_height), ImGuiChildFlags_Borders);
	ImGui::TextDisabled("Original");
	ImGui::Separator();
	render_text_with_topic_highlights(entry.old_text);
	ImGui::EndChild();

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.1f));
	ImGui::Button("##editor_splitter", ImVec2(splitter_width, panel_height));
	ImGui::PopStyleColor(3);
	if (ImGui::IsItemActive())
		split_ratio_ += ImGui::GetIO().MouseDelta.x / available_width;
	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	split_ratio_ = std::clamp(split_ratio_, 0.2f, 0.8f);
	ImGui::SameLine();

	ImGui::BeginChild("##editor_translation", ImVec2(right_width, panel_height), ImGuiChildFlags_Borders);
	ImGui::TextDisabled("Translation");
	ImGui::Separator();

	if (!is_editable)
	{
		ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
		ImGui::TextUnformatted(entry.new_text.c_str());
		ImGui::PopTextWrapPos();
		ImGui::EndChild();
		return;
	}

	auto & mutable_entry = state_.get_user_dict().find(row.type)->second.records[row.record_index];

	bool same_record = (editing_row_ == selected_row_
	    && editing_type_ == row.type
	    && editing_record_index_ == row.record_index);

	if (!same_record)
	{
		if (editing_row_ >= 0 && editing_type_ != tools_t::rec_type_t::unknown)
		{
			std::string committed(edit_buffer_.data());
			auto prev_it = state_.get_user_dict().find(editing_type_);
			if (prev_it != state_.get_user_dict().end() && editing_record_index_ < prev_it->second.records.size())
			{
				auto & prev_entry = prev_it->second.records[editing_record_index_];
				if (committed != prev_entry.new_text)
				{
					history_.record_change(editing_type_, prev_entry.key_text, prev_entry.new_text, committed);
					prev_entry.new_text = committed;
					prev_entry.status = tools_t::status_t::in_progress;
					auto vr = validation_.validate(editing_type_, committed);
					if (vr.level == validation_level_t::error)
						prev_entry.status = tools_t::status_t::has_errors;
					state_.mark_modified(editing_type_, editing_record_index_);
					if (editing_type_ == tools_t::rec_type_t::dial)
						annotations_mgr_.rebuild(state_, merged_base_dict_);
				}
			}
		}

		editing_row_ = selected_row_;
		editing_type_ = row.type;
		editing_record_index_ = row.record_index;
		edit_buffer_.resize(EDIT_BUFFER_SIZE);
		std::fill(edit_buffer_.begin(), edit_buffer_.end(), '\0');
		std::string buf_text;
		buf_text.reserve(mutable_entry.new_text.size());
		for (size_t ci = 0; ci < mutable_entry.new_text.size(); ++ci)
		{
			if (mutable_entry.new_text[ci] == '\r' && ci + 1 < mutable_entry.new_text.size() && mutable_entry.new_text[ci + 1] == '\n')
				continue;
			buf_text += mutable_entry.new_text[ci];
		}
		size_t copy_len = buf_text.size();
		if (copy_len > EDIT_BUFFER_SIZE - 1)
			copy_len = EDIT_BUFFER_SIZE - 1;
		std::copy_n(buf_text.begin(), copy_len, edit_buffer_.begin());
	}

	float input_height = ImGui::GetContentRegionAvail().y;

	ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_AllowTabInput;
	bool changed = ImGui::InputTextMultiline(
	    "##editor_input",
	    edit_buffer_.data(),
	    edit_buffer_.size(),
	    ImVec2(-1, input_height),
	    input_flags);

	if (changed)
	{
		std::string new_value(edit_buffer_.data());
		std::string normalized;
		normalized.reserve(new_value.size());
		for (size_t i = 0; i < new_value.size(); ++i)
		{
			if (new_value[i] == '\n' && (i == 0 || new_value[i - 1] != '\r'))
				normalized += "\r\n";
			else
				normalized += new_value[i];
		}
		if (normalized != mutable_entry.new_text)
		{
			history_.record_change(row.type, mutable_entry.key_text, mutable_entry.new_text, normalized);
			mutable_entry.new_text = normalized;
			mutable_entry.status = tools_t::status_t::in_progress;
			auto vr = validation_.validate(row.type, normalized);
			if (vr.level == validation_level_t::error)
				mutable_entry.status = tools_t::status_t::has_errors;
			state_.mark_modified(row.type, row.record_index);
			if (row.type == tools_t::rec_type_t::dial)
				annotations_mgr_.rebuild(state_, merged_base_dict_);
		}
	}

	ImGui::EndChild();
}

void editor_app_t::render_annotations_tab()
{
	const auto & row = left_rows_[selected_row_];

	const auto & active_dict = (selected_dict_ == selected_dict_t::base)    ? merged_base_dict_
	                           : (selected_dict_ == selected_dict_t::source) ? state_.get_source_dict()
	                                                                         : state_.get_user_dict();

	auto it = active_dict.find(row.type);
	if (it == active_dict.end() || row.record_index >= it->second.records.size())
		return;

	const auto & entry = it->second.records[row.record_index];

	if (row.type == tools_t::rec_type_t::fnam && annotations_mgr_.has_enchantment(entry.key_text))
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "\xe2\x9a\xa1 Enchantment");
		const auto & ench_name = annotations_mgr_.get_enchantment(entry.key_text);
		ImGui::SameLine();
		ImGui::TextUnformatted(ench_name.c_str());
		ImGui::Separator();
	}

	const auto & base_dict = merged_base_dict_;
	auto base_it = base_dict.find(row.type);
	if (base_it != base_dict.end())
	{
		const auto * base_entry = base_it->second.find(entry.key_text);
		if (base_entry != nullptr && !base_entry->new_text.empty())
		{
			ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.6f, 1.0f), "Base proposal:");
			ImGui::SameLine();
			ImGui::TextUnformatted(base_entry->new_text.c_str());
			ImGui::Separator();
		}
	}

	if (row.type == tools_t::rec_type_t::info)
	{
		auto annotations_new = annotations_mgr_.annotate(entry.new_text, row.type);
		auto annotations_old = annotations_mgr_.annotate(entry.old_text, row.type);

		std::vector<const annotation_t *> hyperlinks;
		std::vector<const annotation_t *> glossary;

		for (const auto & ann : annotations_new)
		{
			if (ann.kind == annotation_t::dial_topic)
				hyperlinks.push_back(&ann);
			else if (ann.kind == annotation_t::glossary_term)
				glossary.push_back(&ann);
		}

		for (const auto & ann : annotations_old)
		{
			if (ann.kind == annotation_t::dial_topic)
				hyperlinks.push_back(&ann);
		}

		if (!hyperlinks.empty())
		{
			ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.8f, 1.0f), "Hyperlinks");
			for (const auto * ann : hyperlinks)
			{
				std::string label = ann->old_text + " -> " + ann->new_text;
				if (ImGui::Selectable(label.c_str()))
					ImGui::SetClipboardText(ann->new_text.c_str());
			}
			ImGui::Separator();
		}

		if (!glossary.empty())
		{
			ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.4f, 1.0f), "Glossary");
			for (const auto * ann : glossary)
			{
				std::string label = ann->old_text + " -> " + ann->new_text;
				if (ImGui::Selectable(label.c_str()))
					ImGui::SetClipboardText(ann->new_text.c_str());
			}
			ImGui::Separator();
		}

		auto key_parts = entry.key_text;
		auto sep_pos = key_parts.find('^');
		if (sep_pos != std::string::npos)
		{
			std::string npc_id = key_parts.substr(sep_pos + 1);
			const auto & gender = annotations_mgr_.get_speaker_gender(npc_id);
			if (!gender.empty())
			{
				ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.9f, 1.0f), "Speaker");
				ImGui::SameLine();
				std::string speaker_label = npc_id + " (" + gender + ")";
				ImGui::TextUnformatted(speaker_label.c_str());
			}
		}
	}
}

void editor_app_t::render_history_tab()
{
	const auto & row = left_rows_[selected_row_];

	const auto & active_dict = (selected_dict_ == selected_dict_t::base)    ? merged_base_dict_
	                           : (selected_dict_ == selected_dict_t::source) ? state_.get_source_dict()
	                                                                         : state_.get_user_dict();

	auto it = active_dict.find(row.type);
	if (it == active_dict.end() || row.record_index >= it->second.records.size())
		return;

	const auto & entry = it->second.records[row.record_index];
	auto history = history_.get_history(row.type, entry.key_text);

	if (history.empty())
	{
		ImGui::TextDisabled("No history for this record");
		return;
	}

	bool is_editable = (selected_dict_ == selected_dict_t::user);

	ImGui::BeginChild("HistoryScroll", ImVec2(0, 0), ImGuiChildFlags_None);

	for (size_t ri = 0; ri < history.size(); ++ri)
	{
		size_t i = history.size() - 1 - ri;
		ImGui::PushID(static_cast<int>(i));

		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", history[i].timestamp.c_str());
		if (is_editable)
		{
			ImGui::SameLine();
			if (ImGui::SmallButton("Revert"))
			{
				history_.revert(state_, row.type, entry.key_text, i);
				rebuild_row_data();
				ImGui::PopID();
				break;
			}
		}

		std::string prev = history[i].value;
		if (prev.size() > 80)
			prev = prev.substr(0, 77) + "...";

		std::string next_val;
		if (i + 1 < history.size())
			next_val = history[i + 1].value;
		else
			next_val = entry.new_text;
		if (next_val.size() > 80)
			next_val = next_val.substr(0, 77) + "...";

		ImGui::TextWrapped("Previous: %s", prev.c_str());
		ImGui::TextWrapped("New: %s", next_val.c_str());
		ImGui::Separator();

		ImGui::PopID();
	}

	ImGui::EndChild();
}

void editor_app_t::render_speaker_tab()
{
	const auto & dict = (selected_dict_ == selected_dict_t::base)    ? merged_base_dict_
	                    : (selected_dict_ == selected_dict_t::source) ? state_.get_source_dict()
	                                                                  : state_.get_user_dict();
	auto it = dict.find(tools_t::rec_type_t::info);
	if (it == dict.end() || it->second.records.empty())
	{
		ImGui::TextDisabled("No info records loaded");
		return;
	}

	ImGui::SetNextItemWidth(200.0f);
	ImGui::InputText("##speaker_filter", speaker_filter_buffer_.data(), speaker_filter_buffer_.size());
	ImGui::SameLine();
	ImGui::TextDisabled("Filter");

	const ImGuiTableFlags flags = ImGuiTableFlags_Sortable | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
	                              ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV;

	if (!ImGui::BeginTable("##speaker_table", 3, flags, ImVec2(0, ImGui::GetContentRegionAvail().y)))
		return;

	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Speaker ID", ImGuiTableColumnFlags_DefaultSort);
	ImGui::TableSetupColumn("Speaker Name");
	ImGui::TableSetupColumn("Gender", ImGuiTableColumnFlags_WidthFixed, 60.0f);
	ImGui::TableHeadersRow();

	struct speaker_row_t
	{
		const std::string * speaker;
		const std::string * speaker_name;
		const std::string * gender;
	};

	std::vector<speaker_row_t> visible_rows;
	visible_rows.reserve(it->second.records.size());

	std::string filter_text(speaker_filter_buffer_.data());
	for (auto & c : filter_text)
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

	for (const auto & entry : it->second.records)
	{
		if (entry.speaker.empty() && entry.speaker_name.empty() && entry.gender.empty())
			continue;

		if (!filter_text.empty())
		{
			auto lower = [](const std::string & s)
			{
				std::string r = s;
				for (auto & c : r)
					c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
				return r;
			};

			if (lower(entry.speaker).find(filter_text) == std::string::npos &&
			    lower(entry.speaker_name).find(filter_text) == std::string::npos &&
			    lower(entry.gender).find(filter_text) == std::string::npos)
				continue;
		}

		visible_rows.push_back({&entry.speaker, &entry.speaker_name, &entry.gender});
	}

	ImGuiTableSortSpecs * sort_specs = ImGui::TableGetSortSpecs();
	if (sort_specs != nullptr && sort_specs->SpecsDirty && sort_specs->SpecsCount > 0)
	{
		const auto & spec = sort_specs->Specs[0];
		bool ascending = (spec.SortDirection == ImGuiSortDirection_Ascending);
		int col = spec.ColumnIndex;

		std::sort(visible_rows.begin(), visible_rows.end(),
		    [col, ascending](const speaker_row_t & a, const speaker_row_t & b)
		    {
			    const std::string * va = col == 0 ? a.speaker : (col == 1 ? a.speaker_name : a.gender);
			    const std::string * vb = col == 0 ? b.speaker : (col == 1 ? b.speaker_name : b.gender);
			    int cmp = va->compare(*vb);
			    return ascending ? (cmp < 0) : (cmp > 0);
		    });

		sort_specs->SpecsDirty = false;
	}

	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(visible_rows.size()));

	while (clipper.Step())
	{
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
		{
			const auto & row = visible_rows[i];
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(row.speaker->c_str());

			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(row.speaker_name->c_str());

			ImGui::TableSetColumnIndex(2);
			ImGui::TextUnformatted(row.gender->c_str());
		}
	}

	ImGui::EndTable();
}

void editor_app_t::render_annotations_panel()
{
	ImGui::BeginChild("AnnotationsPanel", ImVec2(0, 180), ImGuiChildFlags_Borders);

	ImGui::Text("Annotations");
	ImGui::Separator();

	if (selected_row_left_ < 0 || selected_row_left_ >= static_cast<int>(left_rows_.size()))
	{
		ImGui::TextDisabled("No record selected");
		ImGui::EndChild();
		return;
	}

	const auto & row = left_rows_[selected_row_];

	const auto & active_dict = (selected_dict_ == selected_dict_t::base)    ? merged_base_dict_
	                           : (selected_dict_ == selected_dict_t::source) ? state_.get_source_dict()
	                                                                         : state_.get_user_dict();

	auto it = active_dict.find(row.type);
	if (it == active_dict.end() || row.record_index >= it->second.records.size())
	{
		ImGui::EndChild();
		return;
	}

	const auto & entry = it->second.records[row.record_index];

	if (row.type == tools_t::rec_type_t::fnam && annotations_mgr_.has_enchantment(entry.key_text))
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "\xe2\x9a\xa1 Enchantment");
		const auto & ench_name = annotations_mgr_.get_enchantment(entry.key_text);
		ImGui::SameLine();
		ImGui::TextUnformatted(ench_name.c_str());
		ImGui::Separator();
	}

	const auto & base_dict = merged_base_dict_;
	auto base_it = base_dict.find(row.type);
	if (base_it != base_dict.end())
	{
		const auto * base_entry = base_it->second.find(entry.key_text);
		if (base_entry != nullptr && !base_entry->new_text.empty())
		{
			ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.6f, 1.0f), "Base proposal:");
			ImGui::SameLine();
			ImGui::TextUnformatted(base_entry->new_text.c_str());
			ImGui::Separator();
		}
	}

	if (entry.status == tools_t::status_t::auto_changed && base_it != base_dict.end())
	{
		const auto * base_entry = base_it->second.find(entry.key_text);
		if (base_entry != nullptr)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), "Changed in base - diff:");
			const std::string & old_text = entry.old_text;
			const std::string & base_text = base_entry->old_text;

			size_t min_len = std::min(old_text.size(), base_text.size());
			size_t diff_start = 0;
			while (diff_start < min_len && old_text[diff_start] == base_text[diff_start])
				++diff_start;

			if (diff_start < old_text.size())
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
				std::string removed = "- " + old_text.substr(diff_start);
				ImGui::TextUnformatted(removed.c_str());
				ImGui::PopStyleColor();
			}

			if (diff_start < base_text.size())
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
				std::string added = "+ " + base_text.substr(diff_start);
				ImGui::TextUnformatted(added.c_str());
				ImGui::PopStyleColor();
			}

			ImGui::Separator();
		}
	}

	auto fuzzy = find_fuzzy_matches(entry.old_text, row.type);
	if (!fuzzy.empty())
	{
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.3f, 1.0f), "Fuzzy matches:");
		for (const auto & match : fuzzy)
		{
			int pct = static_cast<int>(match.similarity * 100.0f);
			std::string label = std::to_string(pct) + "%% " + match.matched_key;
			if (ImGui::TreeNode(label.c_str()))
			{
				ImGui::TextWrapped("%s", match.matched_value.c_str());
				ImGui::TreePop();
			}
		}
		ImGui::Separator();
	}

	if (row.type != tools_t::rec_type_t::info)
	{
		ImGui::EndChild();
		return;
	}

	auto annotations_new = annotations_mgr_.annotate(entry.new_text, row.type);
	auto annotations_old = annotations_mgr_.annotate(entry.old_text, row.type);

	std::vector<const annotation_t *> hyperlinks;
	std::vector<const annotation_t *> glossary;

	for (const auto & ann : annotations_new)
	{
		if (ann.kind == annotation_t::dial_topic)
			hyperlinks.push_back(&ann);
		else if (ann.kind == annotation_t::glossary_term)
			glossary.push_back(&ann);
	}

	for (const auto & ann : annotations_old)
	{
		if (ann.kind == annotation_t::dial_topic)
			hyperlinks.push_back(&ann);
	}

	if (!hyperlinks.empty())
	{
		ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.8f, 1.0f), "Hyperlinks");
		for (const auto * ann : hyperlinks)
		{
			std::string label = ann->old_text + " -> " + ann->new_text;
			if (ImGui::Selectable(label.c_str()))
				ImGui::SetClipboardText(ann->new_text.c_str());
		}
		ImGui::Separator();
	}

	if (!glossary.empty())
	{
		ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.4f, 1.0f), "Glossary");
		for (const auto * ann : glossary)
		{
			std::string label = ann->old_text + " -> " + ann->new_text;
			if (ImGui::Selectable(label.c_str()))
				ImGui::SetClipboardText(ann->new_text.c_str());
		}
		ImGui::Separator();
	}

	auto key_parts = entry.key_text;
	auto sep_pos = key_parts.find('^');
	if (sep_pos != std::string::npos)
	{
		std::string npc_id = key_parts.substr(sep_pos + 1);
		const auto & gender = annotations_mgr_.get_speaker_gender(npc_id);
		if (!gender.empty())
		{
			ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.9f, 1.0f), "Speaker");
			ImGui::SameLine();
			std::string speaker_label = npc_id + " (" + gender + ")";
			ImGui::TextUnformatted(speaker_label.c_str());
		}
	}

	ImGui::EndChild();
}

void editor_app_t::render_history_panel()
{
	ImGui::BeginChild("HistoryPanel", ImVec2(0, 150), ImGuiChildFlags_Borders);

	ImGui::Text("Change History");
	ImGui::Separator();

	if (selected_row_left_ < 0 || selected_row_left_ >= static_cast<int>(left_rows_.size()))
	{
		ImGui::TextDisabled("No record selected");
		ImGui::EndChild();
		return;
	}

	const auto & row = left_rows_[selected_row_];

	const auto & active_dict = (selected_dict_ == selected_dict_t::base)    ? merged_base_dict_
	                           : (selected_dict_ == selected_dict_t::source) ? state_.get_source_dict()
	                                                                         : state_.get_user_dict();

	auto it = active_dict.find(row.type);
	if (it == active_dict.end() || row.record_index >= it->second.records.size())
	{
		ImGui::EndChild();
		return;
	}

	const auto & entry = it->second.records[row.record_index];
	auto history = history_.get_history(row.type, entry.key_text);

	if (history.empty())
	{
		ImGui::TextDisabled("No history for this record");
		ImGui::EndChild();
		return;
	}

	for (size_t i = 0; i < history.size(); ++i)
	{
		ImGui::PushID(static_cast<int>(i));

		std::string truncated = history[i].value;
		if (truncated.size() > 60)
			truncated = truncated.substr(0, 57) + "...";

		std::string label = history[i].timestamp + " - " + truncated;

		if (ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
		{
			if (selected_dict_ == selected_dict_t::user && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				history_.revert(state_, row.type, entry.key_text, i);
				rebuild_row_data();
			}
		}

		if (selected_dict_ == selected_dict_t::user)
		{
			ImGui::SameLine();
			if (ImGui::SmallButton("Revert"))
			{
				history_.revert(state_, row.type, entry.key_text, i);
				rebuild_row_data();
			}
		}

		ImGui::PopID();
	}

	ImGui::EndChild();
}

void editor_app_t::render_dialogs()
{
	if (show_quit_dialog_)
		render_quit_dialog();

	if (show_base_dict_config_)
		render_base_dict_config();
}

void editor_app_t::render_quit_dialog()
{
	ImGui::OpenPopup("Unsaved Changes");

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	ImGui::Text("You have unsaved changes. What would you like to do?");
	ImGui::Separator();

	if (ImGui::Button("Save", ImVec2(120, 0)))
	{
		if (state_.get_user_path().empty())
		{
			auto path = show_save_file_dialog("Save User Dictionary");
			if (!path.empty())
				save_user_dict_as_encoded(path);
		}
		else
		{
			save_user_dict_encoded();
		}
		quit_requested_ = true;
		show_quit_dialog_ = false;
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();

	if (ImGui::Button("Discard", ImVec2(120, 0)))
	{
		quit_requested_ = true;
		show_quit_dialog_ = false;
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel", ImVec2(120, 0)))
	{
		show_quit_dialog_ = false;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}

void editor_app_t::render_base_dict_config()
{
	ImGui::SetNextWindowSize(ImVec2(500, 350), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Base Dictionary Configuration", &show_base_dict_config_))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Base dictionary paths (first-wins merge order):");
	ImGui::Separator();

	int to_remove = -1;
	int to_move_up = -1;
	int to_move_down = -1;

	for (int i = 0; i < static_cast<int>(config_.base_dict_paths.size()); ++i)
	{
		ImGui::PushID(i);

		ImGui::TextUnformatted(config_.base_dict_paths[i].c_str());

		ImGui::SameLine();
		if (ImGui::SmallButton("Up") && i > 0)
			to_move_up = i;

		ImGui::SameLine();
		if (ImGui::SmallButton("Down") && i < static_cast<int>(config_.base_dict_paths.size()) - 1)
			to_move_down = i;

		ImGui::SameLine();
		if (ImGui::SmallButton("Remove"))
			to_remove = i;

		ImGui::PopID();
	}

	if (to_move_up >= 0)
		std::swap(config_.base_dict_paths[to_move_up], config_.base_dict_paths[to_move_up - 1]);

	if (to_move_down >= 0)
		std::swap(config_.base_dict_paths[to_move_down], config_.base_dict_paths[to_move_down + 1]);

	if (to_remove >= 0)
		config_.base_dict_paths.erase(config_.base_dict_paths.begin() + to_remove);

	ImGui::Separator();

	if (ImGui::Button("Add..."))
	{
		auto path = show_open_file_dialog("Add Base Dictionary");
		if (!path.empty())
			config_.base_dict_paths.push_back(path);
	}

	ImGui::SameLine();

	if (ImGui::Button("Reload"))
	{
		reload_base_dicts();
		annotations_mgr_.rebuild(state_, merged_base_dict_);
		config_.save(config_path_);
	}

	ImGui::End();
}

std::string editor_app_t::show_open_file_dialog(const char * title) const
{
	char filename[MAX_PATH] = {};

	OPENFILENAMEA ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFilter = "Dictionary Files (*.json;*.xml)\0*.json;*.xml\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = title;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

	if (!GetOpenFileNameA(&ofn))
		return {};

	return std::string(filename);
}

std::string editor_app_t::show_save_file_dialog(const char * title) const
{
	char filename[MAX_PATH] = {};

	OPENFILENAMEA ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = nullptr;
	ofn.lpstrFilter = "Dictionary Files (*.json;*.xml)\0*.json;*.xml\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = title;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
	ofn.lpstrDefExt = "json";

	if (!GetSaveFileNameA(&ofn))
		return {};

	return std::string(filename);
}

void editor_app_t::render_splitter_horizontal(float & height, float min_h, float max_h)
{
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.6f, 0.6f, 0.6f, 0.1f));
	ImGui::Button("##hsplitter", ImVec2(-1, 6.0f));
	ImGui::PopStyleColor(3);

	if (ImGui::IsItemActive())
	{
		height -= ImGui::GetIO().MouseDelta.y;
		height = std::clamp(height, min_h, max_h);
	}

	if (ImGui::IsItemHovered())
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
}

float editor_app_t::clamp_sidebar_width(float requested, float window_width) const
{
	constexpr float min_sidebar = 150.0f;
	constexpr float max_ratio = 0.3f;
	constexpr float min_main = 100.0f;

	float max_sidebar = window_width * max_ratio;
	float absolute_max = window_width - min_main;
	float effective_max = std::min(max_sidebar, absolute_max);

	return std::clamp(requested, min_sidebar, effective_max);
}

float editor_app_t::clamp_bottom_height(float requested, float available_height) const
{
	constexpr float min_bottom = 100.0f;
	constexpr float max_ratio = 0.5f;
	constexpr float min_main = 100.0f;

	float max_bottom = available_height * max_ratio;
	float absolute_max = available_height - min_main;
	float effective_max = std::min(max_bottom, absolute_max);

	return std::clamp(requested, min_bottom, effective_max);
}

void editor_app_t::reencode_dict(tools_t::dict_t & dict, codepage_t old_cp, codepage_t new_cp)
{
	for (auto & [type, chapter] : dict)
	{
		for (auto & entry : chapter.records)
		{
			std::string raw_key = encode_from_utf8(entry.key_text, old_cp);
			entry.key_text = decode_to_utf8(raw_key, new_cp);

			std::string raw_old = encode_from_utf8(entry.old_text, old_cp);
			entry.old_text = decode_to_utf8(raw_old, new_cp);

			std::string raw_new = encode_from_utf8(entry.new_text, old_cp);
			entry.new_text = decode_to_utf8(raw_new, new_cp);
		}

		chapter.index.clear();
		for (size_t i = 0; i < chapter.records.size(); ++i)
			chapter.index[chapter.records[i].key_text] = i;
	}
}

void editor_app_t::decode_dict_from_codepage(tools_t::dict_t & dict, codepage_t cp)
{
	for (auto & [type, chapter] : dict)
	{
		for (auto & entry : chapter.records)
		{
			entry.key_text = decode_to_utf8(entry.key_text, cp);
			entry.old_text = decode_to_utf8(entry.old_text, cp);
			entry.new_text = decode_to_utf8(entry.new_text, cp);
		}

		chapter.index.clear();
		for (size_t i = 0; i < chapter.records.size(); ++i)
			chapter.index[chapter.records[i].key_text] = i;
	}
}

codepage_t editor_app_t::active_codepage() const
{
	return supported_codepages[encoding_index_];
}

void editor_app_t::save_user_dict_encoded()
{
	codepage_t cp = active_codepage();
	auto & dict = state_.get_user_dict();
	for (auto & [type, chapter] : dict)
	{
		for (auto & entry : chapter.records)
		{
			entry.key_text = encode_from_utf8(entry.key_text, cp);
			entry.old_text = encode_from_utf8(entry.old_text, cp);
			entry.new_text = encode_from_utf8(entry.new_text, cp);
		}
	}

	state_.save_user_dict();

	for (auto & [type, chapter] : dict)
	{
		for (auto & entry : chapter.records)
		{
			entry.key_text = decode_to_utf8(entry.key_text, cp);
			entry.old_text = decode_to_utf8(entry.old_text, cp);
			entry.new_text = decode_to_utf8(entry.new_text, cp);
		}
	}
}

void editor_app_t::save_user_dict_as_encoded(const std::string & path)
{
	codepage_t cp = active_codepage();
	auto & dict = state_.get_user_dict();
	for (auto & [type, chapter] : dict)
	{
		for (auto & entry : chapter.records)
		{
			entry.key_text = encode_from_utf8(entry.key_text, cp);
			entry.old_text = encode_from_utf8(entry.old_text, cp);
			entry.new_text = encode_from_utf8(entry.new_text, cp);
		}
	}

	state_.save_user_dict_as(path);

	for (auto & [type, chapter] : dict)
	{
		for (auto & entry : chapter.records)
		{
			entry.key_text = decode_to_utf8(entry.key_text, cp);
			entry.old_text = decode_to_utf8(entry.old_text, cp);
			entry.new_text = decode_to_utf8(entry.new_text, cp);
		}
	}
}

void editor_app_t::render_splitter_vertical(float & width, float min_w, float max_w)
{
	constexpr float thickness = 6.0f;
	float height = ImGui::GetContentRegionAvail().y;

	ImGui::SameLine();
	ImGui::InvisibleButton("##vsplitter", ImVec2(thickness, height));

	if (ImGui::IsItemActive())
		width = std::clamp(width + ImGui::GetIO().MouseDelta.x, min_w, max_w);

	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

	ImGui::SameLine();
}


void editor_app_t::reload_base_dicts()
{
	merged_base_dict_ = tools_t::initialize_dict();

	if (config_.base_dict_paths.empty())
		return;

	dict_merger_t merger(config_.base_dict_paths);
	merged_base_dict_ = merger.get_dict();
	decode_dict_from_codepage(merged_base_dict_, active_codepage());
}

void editor_app_t::save_current_filters()
{
	int idx = static_cast<int>(selected_dict_);
	filter_per_dict_[idx].status_filter = status_filter_;
	filter_per_dict_[idx].type_filter = type_filter_;
	filter_per_dict_[idx].sidebar_active_type = sidebar_active_type_;
}

void editor_app_t::restore_filters_for(selected_dict_t dict)
{
	int idx = static_cast<int>(dict);
	status_filter_ = filter_per_dict_[idx].status_filter;
	type_filter_ = filter_per_dict_[idx].type_filter;
	sidebar_active_type_ = filter_per_dict_[idx].sidebar_active_type;
}

static size_t levenshtein_distance(const std::string & a, const std::string & b)
{
	size_t m = a.size();
	size_t n = b.size();

	if (m == 0)
		return n;
	if (n == 0)
		return m;

	std::vector<size_t> prev(n + 1);
	std::vector<size_t> curr(n + 1);

	for (size_t j = 0; j <= n; ++j)
		prev[j] = j;

	for (size_t i = 1; i <= m; ++i)
	{
		curr[0] = i;
		for (size_t j = 1; j <= n; ++j)
		{
			size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
			curr[j] = std::min({ curr[j - 1] + 1, prev[j] + 1, prev[j - 1] + cost });
		}
		std::swap(prev, curr);
	}

	return prev[n];
}

std::vector<editor_app_t::fuzzy_match_t> editor_app_t::find_fuzzy_matches(
    const std::string & text,
    tools_t::rec_type_t type,
    size_t max_results) const
{
	std::vector<fuzzy_match_t> matches;

	if (text.empty())
		return matches;

	auto it = merged_base_dict_.find(type);
	if (it == merged_base_dict_.end())
		return matches;

	for (const auto & entry : it->second.records)
	{
		if (entry.old_text.empty())
			continue;
		if (entry.old_text == text)
			continue;

		size_t max_len = std::max(text.size(), entry.old_text.size());
		size_t dist = levenshtein_distance(text, entry.old_text);
		float similarity = 1.0f - static_cast<float>(dist) / static_cast<float>(max_len);

		if (similarity < 0.5f)
			continue;

		fuzzy_match_t match;
		match.matched_key = entry.key_text;
		match.matched_value = entry.new_text;
		match.similarity = similarity;
		matches.push_back(match);
	}

	std::sort(
	    matches.begin(),
	    matches.end(),
	    [](const fuzzy_match_t & a, const fuzzy_match_t & b) { return a.similarity > b.similarity; });

	if (matches.size() > max_results)
		matches.resize(max_results);

	return matches;
}
