#define NOMINMAX

#include "editor_app.hpp"
#include "encoding_utils.hpp"
#include "imgui.h"
#include "status_colors.hpp"

#include <algorithm>
#include <array>
#include <cctype>

#include <Windows.h>
#include <commdlg.h>
#include <Richedit.h>
#include <SDL_syswm.h>

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

	for (const auto & path : config_.user_dict_paths)
	{
		int idx = workspace_.load_dict(path, dict_kind_t::user);
		if (idx < 0)
			tools_t::add_log("[warn] cannot load user dict: \"" + path + "\"\r\n");
		else
			decode_dict_from_codepage(workspace_.get_slot(idx)->data, active_codepage());
	}

	for (const auto & path : config_.base_dict_paths)
	{
		int idx = workspace_.load_dict(path, dict_kind_t::base);
		if (idx < 0)
			tools_t::add_log("[warn] cannot load base dict: \"" + path + "\"\r\n");
		else
			decode_dict_from_codepage(workspace_.get_slot(idx)->data, active_codepage());
	}

	if (config_.active_dict_index >= 0 && config_.active_dict_index < workspace_.slot_count())
		workspace_.set_active(config_.active_dict_index);

	if (!config_.spell_check_aff.empty() && !config_.spell_check_dic.empty())
		spell_checker_.load(config_.spell_check_aff, config_.spell_check_dic);

	SDL_SysWMinfo wm_info = {};
	SDL_VERSION(&wm_info.version);
	SDL_GetWindowWMInfo(window_, &wm_info);
	create_richedit(wm_info.info.win.window);

	rebuild_annotations();
	rebuild_row_data();
}

void editor_app_t::frame()
{
	std::string title = "yampt.gui";
	if (workspace_.has_any_unsaved())
		title += " *";
	SDL_SetWindowTitle(window_, title.c_str());

	const ImGuiViewport * viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);

	ImGuiWindowFlags window_flags =
	    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
	    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
	    ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

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
	    "PanelArea",
	    ImVec2(0, panel_area_height),
	    ImGuiChildFlags_None,
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
		if (main_panel_height < 30.0f)
			main_panel_height = 30.0f;
	}

	ImGui::BeginChild(
	    "MainPanelArea",
	    ImVec2(0, main_panel_height),
	    ImGuiChildFlags_None,
	    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	render_main_panel();
	ImGui::EndChild();

	if (bottom_visible_)
	{
		float max_h = clamp_bottom_height(panel_area_height, panel_area_height);
		render_splitter_horizontal(bottom_height_, 50.0f, max_h);
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
	if (richedit_hwnd_)
	{
		DestroyWindow(richedit_hwnd_);
		richedit_hwnd_ = nullptr;
	}

	config_.split_ratio = split_ratio_;
	config_.sidebar_width = sidebar_width_;
	config_.bottom_height = bottom_height_;
	config_.sidebar_visible = sidebar_visible_;
	config_.bottom_visible = bottom_visible_;
	config_.encoding_index = encoding_index_;

	config_.user_dict_paths.clear();
	config_.base_dict_paths.clear();
	for (int i = 0; i < workspace_.slot_count(); ++i)
	{
		const auto * slot = workspace_.get_slot(i);
		if (!slot)
			continue;
		if (workspace_.is_user_slot(i))
			config_.user_dict_paths.push_back(slot->path);
		else if (workspace_.is_base_slot(i))
			config_.base_dict_paths.push_back(slot->path);
	}
	config_.active_dict_index = workspace_.get_active_index();

	config_.save(config_path_);
}

bool editor_app_t::wants_quit() const
{
	return quit_requested_;
}

void editor_app_t::request_quit()
{
	if (workspace_.has_any_unsaved())
		show_quit_dialog_ = true;
	else
		quit_requested_ = true;
}

std::string editor_app_t::make_lookup_key(tools_t::rec_type_t type, const std::string & key)
{
	return std::to_string(static_cast<int>(type)) + "|" + key;
}

void editor_app_t::rebuild_annotations()
{
	std::vector<dict_source_t> sources;
	for (int i = 0; i < workspace_.slot_count(); ++i)
	{
		const auto * slot = workspace_.get_slot(i);
		if (!slot)
			continue;
		auto pos = slot->path.find_last_of("\\/");
		std::string name = (pos != std::string::npos) ? slot->path.substr(pos + 1) : slot->path;
		sources.push_back({ &slot->data, name });
	}
	annotations_mgr_.rebuild(sources);
}

void editor_app_t::rebuild_row_data()
{
	left_rows_.clear();
	left_lookup_.clear();

	auto * slot = workspace_.get_active_slot();
	if (!slot)
		return;

	const auto & dict = slot->data;

	int idx = 0;
	for (const auto & [type, chapter] : dict)
	{
		if (type_filter_.count(type) == 0)
			continue;

		for (size_t i = 0; i < chapter.records.size(); ++i)
		{
			if (!status_filter_.empty())
			{
				const auto & s = chapter.records[i].status;
				bool match = s.empty() ? status_filter_.count("untranslated") > 0 : status_filter_.count(s) > 0;
				if (!match)
					continue;
			}

			left_rows_.push_back({ type, i });
			left_lookup_[make_lookup_key(type, chapter.records[i].key_text)] = idx;
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
				int prev_count = workspace_.slot_count();
				int idx = workspace_.load_dict(path, dict_kind_t::user);
				if (idx >= 0 && workspace_.slot_count() > prev_count)
					decode_dict_from_codepage(workspace_.get_slot(idx)->data, active_codepage());
				auto * active_slot = workspace_.get_active_slot();
				if (active_slot)
					rebuild_annotations();
				rebuild_row_data();
			}
		}

		if (ImGui::MenuItem("Open Base Dict..."))
		{
			auto path = show_open_file_dialog("Open Base Dictionary");
			if (!path.empty())
			{
				int prev_count = workspace_.slot_count();
				int idx = workspace_.load_dict(path, dict_kind_t::base);
				if (idx >= 0 && workspace_.slot_count() > prev_count)
					decode_dict_from_codepage(workspace_.get_slot(idx)->data, active_codepage());
				rebuild_row_data();
			}
		}

		ImGui::Separator();

		if (ImGui::MenuItem("Save", "Ctrl+S"))
		{
			commit_richedit_text();
			save_user_dict_encoded();
		}

		if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
		{
			auto path = show_save_file_dialog("Save User Dictionary As");
			if (!path.empty())
			{
				commit_richedit_text();
				save_user_dict_as_encoded(path);
			}
		}

		if (ImGui::MenuItem("Save All"))
		{
			commit_richedit_text();
			save_all_encoded();
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
		auto * search_slot = workspace_.get_active_slot();
		if (search_slot)
			search_.find_all(search_slot->data, type_filter_);
	}

	ImGui::SameLine();
	if (ImGui::Checkbox("Case sensitive", &search_case_sensitive_))
	{
		search_.set_query(search_buffer_.data(), search_case_sensitive_);
		auto * search_slot = workspace_.get_active_slot();
		if (search_slot)
			search_.find_all(search_slot->data, type_filter_);
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
					auto * enc_slot = workspace_.get_active_slot();
					if (enc_slot)
					{
						reencode_dict(enc_slot->data, old_cp, new_cp);
						rebuild_annotations();
					}
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
		auto * toolbar_slot = workspace_.get_active_slot();
		if (toolbar_slot)
		{
			auto it = toolbar_slot->data.find(type);
			if (it != toolbar_slot->data.end())
				count = it->second.records.size();
		}

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
		commit_richedit_text();
		save_user_dict_encoded();
	}

	if (ImGui::IsKeyPressed(ImGuiKey_F5) || (GetAsyncKeyState(VK_F5) & 1))
	{
		if (selected_row_ >= 0 && selected_row_ < static_cast<int>(left_rows_.size()))
		{
			const auto & f5_row = left_rows_[selected_row_];
			auto * f5_slot = workspace_.get_active_slot();
			if (f5_slot)
			{
				auto f5_it = f5_slot->data.find(f5_row.type);
				if (f5_it != f5_slot->data.end() && f5_row.record_index < f5_it->second.records.size())
				{
					const auto & f5_entry = f5_it->second.records[f5_row.record_index];

					CHARRANGE saved_sel;
					SendMessageA(richedit_hwnd_, EM_EXGETSEL, 0, (LPARAM)&saved_sel);

					std::string current_text = get_richedit_text();
					set_richedit_text(current_text);
					highlight_richedit_hyperlinks(current_text, f5_row.type, f5_entry.old_text);

					SendMessageA(richedit_hwnd_, EM_EXSETSEL, 0, (LPARAM)&saved_sel);
				}
			}
		}
	}

	ImGui::EndChild();
}

void editor_app_t::render_sidebar()
{
	if (!sidebar_visible_)
		return;

	ImGui::BeginChild("Sidebar", ImVec2(sidebar_width_, 0), ImGuiChildFlags_Borders);

	int active_index = workspace_.get_active_index();

	ImGui::SeparatorText("User Dicts");

	for (int i = 0; i < workspace_.slot_count(); ++i)
	{
		if (!workspace_.is_user_slot(i))
			continue;

		const auto * slot = workspace_.get_slot(i);
		if (!slot)
			continue;

		const auto & path = slot->path;
		auto pos = path.find_last_of("\\/");
		std::string filename = (pos != std::string::npos) ? path.substr(pos + 1) : path;

		std::string label = slot->dirty ? "* " + filename : filename;

		ImGui::PushID(i);
		if (ImGui::Selectable(label.c_str(), i == active_index))
		{
			workspace_.set_active(i);
			rebuild_annotations();
			rebuild_row_data();
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", path.c_str());

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Save"))
				save_slot_encoded(i);
			if (ImGui::MenuItem("Save As..."))
			{
				auto save_path = show_save_file_dialog("Save Dictionary As");
				if (!save_path.empty())
					save_slot_as_encoded(i, save_path);
			}
			if (ImGui::MenuItem("Unload"))
			{
				if (slot->dirty)
				{
					pending_unload_index_ = i;
					show_unload_confirm_ = true;
				}
				else
				{
					workspace_.unload_dict(i);
					rebuild_row_data();
				}
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();
	}

	ImGui::SeparatorText("Base Dicts");

	for (int i = 0; i < workspace_.slot_count(); ++i)
	{
		if (!workspace_.is_base_slot(i))
			continue;

		const auto * slot = workspace_.get_slot(i);
		if (!slot)
			continue;

		const auto & path = slot->path;
		auto pos = path.find_last_of("\\/");
		std::string filename = (pos != std::string::npos) ? path.substr(pos + 1) : path;

		ImGui::PushID(i);
		if (ImGui::Selectable(filename.c_str(), i == active_index))
		{
			workspace_.set_active(i);
			rebuild_annotations();
			rebuild_row_data();
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("%s", path.c_str());

		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Remove"))
			{
				workspace_.unload_dict(i);
				rebuild_row_data();
			}
			ImGui::EndPopup();
		}

		ImGui::PopID();
	}

	ImGui::EndChild();
}

void editor_app_t::render_status_summary_bar()
{
	static const char * status_names[] = { "untranslated",    "missing",         "duplicate",       "matched_by_coords",
		                                   "matched_by_info", "matched_by_name", "wilderness",      "region",
		                                   "auto_identical",  "auto_base",       "auto_translated", "auto_heuristic",
		                                   "auto_changed",    "in_progress",     "translated",      "has_errors" };
	static const char * status_labels[] = { "Untranslated",   "Missing",      "Duplicate",       "Matched Coords",
		                                    "Matched Info",   "Matched Name", "Wilderness",      "Region",
		                                    "Auto Identical", "Auto Base",    "Auto Translated", "Auto Heuristic",
		                                    "Auto Changed",   "In Progress",  "Translated",      "Has Errors" };
	static constexpr size_t status_count = 16;

	int counts[status_count] = {};

	auto * summary_slot = workspace_.get_active_slot();

	if (summary_slot)
	{
		for (const auto & [type, chapter] : summary_slot->data)
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

void editor_app_t::render_main_panel()
{
	ImGui::BeginChild("MainPanel", ImVec2(0, 0), ImGuiChildFlags_Borders);

	auto * slot = workspace_.get_active_slot();
	if (!slot)
	{
		ImGui::TextDisabled("No dictionary loaded");
		ImGui::EndChild();
		return;
	}

	bool is_editable = workspace_.is_user_slot(workspace_.get_active_index());

	const auto & active_dict = slot->data;

	const auto & rows = left_rows_;
	int & selected_row = selected_row_;
	int & scroll_to_row = scroll_to_row_;

	const ImGuiTableFlags flags = ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
	                              ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Sortable;

	if (!ImGui::BeginTable("##main_table", 6, flags))
	{
		ImGui::EndChild();
		return;
	}

	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn(
	    "Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort, 50.0f);
	ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, config_.column_widths[0]);
	ImGui::TableSetupColumn("Original", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Translation", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, config_.column_widths[3]);
	ImGui::TableSetupColumn("##validation", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 24.0f);
	ImGui::TableHeadersRow();

	ImGuiTableSortSpecs * sort_specs = ImGui::TableGetSortSpecs();
	if (sort_specs != nullptr && sort_specs->SpecsDirty && sort_specs->SpecsCount > 0)
	{
		const auto & spec = sort_specs->Specs[0];
		bool ascending = (spec.SortDirection == ImGuiSortDirection_Ascending);
		int col = spec.ColumnIndex;

		std::sort(
		    left_rows_.begin(),
		    left_rows_.end(),
		    [&](const row_ref_t & a, const row_ref_t & b)
		{
			auto it_a = active_dict.find(a.type);
			auto it_b = active_dict.find(b.type);
			if (it_a == active_dict.end() || it_b == active_dict.end())
				return false;
			if (a.record_index >= it_a->second.records.size() || b.record_index >= it_b->second.records.size())
				return false;
			const auto & ea = it_a->second.records[a.record_index];
			const auto & eb = it_b->second.records[b.record_index];

			int cmp = 0;
			if (col == 0)
			{
				std::string ta = tools_t::type_to_str(a.type);
				std::string tb = tools_t::type_to_str(b.type);
				cmp = ta.compare(tb);
			}
			else if (col == 1)
				cmp = ea.key_text.compare(eb.key_text);
			else if (col == 2)
				cmp = ea.old_text.compare(eb.old_text);
			else if (col == 3)
				cmp = ea.new_text.compare(eb.new_text);
			else if (col == 4)
				cmp = ea.status.compare(eb.status);

			return ascending ? (cmp < 0) : (cmp > 0);
		});

		left_lookup_.clear();
		for (int i = 0; i < static_cast<int>(left_rows_.size()); ++i)
		{
			auto it2 = active_dict.find(left_rows_[i].type);
			if (it2 != active_dict.end() && left_rows_[i].record_index < it2->second.records.size())
				left_lookup_[make_lookup_key(
				    left_rows_[i].type, it2->second.records[left_rows_[i].record_index].key_text)] = i;
		}

		sort_specs->SpecsDirty = false;
	}

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
			ImGui::PushID(i);

			if (ImGui::Selectable(tools_t::type_to_str(row_ref.type).c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, 0)))
			{
				selected_row = i;
				selected_row_left_ = i;
			}

			if (is_editable && ImGui::BeginPopupContextItem("##row_ctx"))
			{
				if (ImGui::BeginMenu("Set Status"))
				{
					static const char * ctx_status_names[] = {
						"untranslated", "in_progress", "translated", "has_errors"
					};
					static const char * ctx_status_labels[] = {
						"Untranslated", "In Progress", "Translated", "Has Errors"
					};

					for (int s = 0; s < 4; ++s)
					{
						if (ImGui::MenuItem(ctx_status_labels[s]))
						{
							auto mit = slot->data.find(row_ref.type);
							if (mit != slot->data.end() && row_ref.record_index < mit->second.records.size())
							{
								auto & rec = mit->second.records[row_ref.record_index];
								std::string old_status = rec.status;
								rec.status = ctx_status_names[s];
								slot->dirty = true;
								slot->modified_records.insert({ row_ref.type, row_ref.record_index });
								history_.record_change(row_ref.type, entry.key_text, old_status, rec.status);
							}
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}

			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(entry.key_text.c_str());

			ImGui::TableSetColumnIndex(2);
			{
				const char * end = entry.old_text.c_str();
				while (*end && *end != '\r' && *end != '\n')
					++end;
				ImGui::TextUnformatted(entry.old_text.c_str(), end);
			}

			ImGui::TableSetColumnIndex(3);
			{
				const char * end = entry.new_text.c_str();
				while (*end && *end != '\r' && *end != '\n')
					++end;
				ImGui::TextUnformatted(entry.new_text.c_str(), end);
			}

			ImGui::TableSetColumnIndex(4);
			ImVec4 status_color = get_status_color(entry.status);
			ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::ColorConvertFloat4ToU32(status_color));

			std::string status_label = entry.status.empty() ? "(none)" : entry.status;
			if (is_editable)
			{
				if (ImGui::Selectable(status_label.c_str(), false, ImGuiSelectableFlags_None))
					ImGui::OpenPopup("##status_popup");

				if (ImGui::BeginPopup("##status_popup"))
				{
					static const char * popup_status_names[] = {
						"untranslated", "in_progress", "translated", "has_errors"
					};
					static const char * popup_status_labels[] = {
						"Untranslated", "In Progress", "Translated", "Has Errors"
					};

					for (int s = 0; s < 4; ++s)
					{
						if (ImGui::Selectable(popup_status_labels[s]))
						{
							auto mit = slot->data.find(row_ref.type);
							if (mit != slot->data.end() && row_ref.record_index < mit->second.records.size())
							{
								auto & rec = mit->second.records[row_ref.record_index];
								std::string old_status = rec.status;
								rec.status = popup_status_names[s];
								slot->dirty = true;
								slot->modified_records.insert({ row_ref.type, row_ref.record_index });
								history_.record_change(row_ref.type, entry.key_text, old_status, rec.status);
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

			ImGui::TableSetColumnIndex(5);
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
					    ImVec2(rect_x, rect_y), ImVec2(line_x, rect_y + ImGui::GetTextLineHeight()), highlight_u32);
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
				    ImVec2(rect_x, rect_y), ImVec2(line_x, rect_y + ImGui::GetTextLineHeight()), highlight_u32);
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
		    ImVec2(rect_x, rect_y), ImVec2(line_x, rect_y + ImGui::GetTextLineHeight()), highlight_u32);
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
	ImU32 hyperlink_color = IM_COL32(70, 130, 200, 60);
	ImU32 glossary_color = IM_COL32(70, 180, 70, 60);
	ImFont * font = ImGui::GetFont();
	float font_size = ImGui::GetFontSize();

	std::vector<int> highlight_count(text.size(), 0);

	for (const auto & ann : annotations)
	{
		if (ann.start >= text.size() || ann.end > text.size())
			continue;

		bool overlaps = false;
		for (size_t i = ann.start; i < ann.end; ++i)
		{
			if (highlight_count[i] >= 3)
			{
				overlaps = true;
				break;
			}
		}
		if (overlaps)
			continue;

		for (size_t i = ann.start; i < ann.end; ++i)
			++highlight_count[i];

		ImU32 color = (ann.kind == annotation_t::glossary_term) ? glossary_color : hyperlink_color;

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
			const char * line_end = font->CalcWordWrapPositionA(1.0f, s, text_end, wrap_width);
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

		draw_list->AddRectFilled(rect_min, rect_max, color);
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
			    ImVec2(x, base_y), ImVec2(mid_x, base_y + y_offset), ImVec2(next_x, base_y), underline_color, 1.5f);

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
	{
		if (richedit_visible_ && richedit_hwnd_)
		{
			ShowWindow(richedit_hwnd_, SW_HIDE);
			richedit_visible_ = false;
		}
		return;
	}

	float total_height = bottom_height_;
	float splitter_h = 6.0f;
	float editor_height = total_height - info_height_ - splitter_h;
	if (editor_height < 50.0f)
		editor_height = 50.0f;

	ImGui::BeginChild("EditorPanel", ImVec2(0, editor_height), ImGuiChildFlags_Borders);
	if (selected_row_ < 0 || selected_row_ >= static_cast<int>(left_rows_.size()))
	{
		ImGui::TextDisabled("No record selected");
		if (richedit_visible_ && richedit_hwnd_)
		{
			ShowWindow(richedit_hwnd_, SW_HIDE);
			richedit_visible_ = false;
		}
	}
	else
	{
		render_editor_tab();
	}
	ImGui::EndChild();

	ImGui::PushID("info_splitter");
	render_splitter_horizontal(info_height_, 50.0f, total_height - 80.0f);
	ImGui::PopID();

	render_info_panel();
}

void editor_app_t::render_info_panel()
{
	ImGui::BeginChild("InfoPanel", ImVec2(0, info_height_), ImGuiChildFlags_Borders);

	if (!ImGui::BeginTabBar("InfoTabs"))
	{
		ImGui::EndChild();
		return;
	}

	if (ImGui::BeginTabItem("Annotations"))
	{
		if (selected_row_ < 0 || selected_row_ >= static_cast<int>(left_rows_.size()))
			ImGui::TextDisabled("No record selected");
		else
			render_annotations_tab();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("History"))
	{
		if (selected_row_ < 0 || selected_row_ >= static_cast<int>(left_rows_.size()))
			ImGui::TextDisabled("No record selected");
		else
			render_history_tab();
		ImGui::EndTabItem();
	}

	if (ImGui::BeginTabItem("Speaker"))
	{
		render_speaker_tab();
		ImGui::EndTabItem();
	}

	ImGui::EndTabBar();
	ImGui::EndChild();
}

void editor_app_t::render_editor_tab()
{
	const auto & row = left_rows_[selected_row_];

	auto * slot = workspace_.get_active_slot();
	if (!slot)
		return;

	const auto & active_dict = slot->data;
	bool is_editable = workspace_.is_user_slot(workspace_.get_active_index());

	auto it = active_dict.find(row.type);
	if (it == active_dict.end() || row.record_index >= it->second.records.size())
		return;

	const auto & entry = it->second.records[row.record_index];
	float available_width = ImGui::GetContentRegionAvail().x;
	float splitter_width = 6.0f;
	float left_width = available_width * split_ratio_ - splitter_width * 0.5f;
	float right_width = available_width - left_width - splitter_width - 16.0f;
	float panel_height = ImGui::GetContentRegionAvail().y;

	ImGui::BeginChild("##editor_original", ImVec2(left_width, panel_height), ImGuiChildFlags_Borders);
	ImGui::TextDisabled("Original");
	ImGui::Separator();
	if (row.type == tools_t::rec_type_t::info)
		render_text_with_topic_highlights(entry.old_text);
	else
		ImGui::TextWrapped("%s", entry.old_text.c_str());
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

	bool same_record =
	    (editing_row_ == selected_row_ && editing_type_ == row.type && editing_record_index_ == row.record_index);

	if (!same_record)
	{
		if (editing_row_ >= 0 && editing_type_ != tools_t::rec_type_t::unknown)
			commit_richedit_text();

		editing_row_ = selected_row_;
		editing_type_ = row.type;
		editing_record_index_ = row.record_index;

		richedit_ignore_change_ = true;
		set_richedit_text(entry.new_text);
		if (row.type == tools_t::rec_type_t::info)
			highlight_richedit_hyperlinks(entry.new_text, row.type, entry.old_text);
		richedit_ignore_change_ = false;
	}

	SendMessageA(richedit_hwnd_, EM_SETREADONLY, is_editable ? FALSE : TRUE, 0);

	ImVec2 cursor_pos = ImGui::GetCursorScreenPos();
	ImVec2 region = ImGui::GetContentRegionAvail();
	position_richedit(cursor_pos.x, cursor_pos.y, region.x, region.y);

	ImGui::Dummy(region);

	ImGui::EndChild();
}

void editor_app_t::render_annotations_tab()
{
	const auto & row = left_rows_[selected_row_];

	auto * slot = workspace_.get_active_slot();
	if (!slot)
		return;

	const auto & active_dict = slot->data;

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

	static const tools_t::dict_t empty_base_dict;
	const auto & base_dict = empty_base_dict;
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
			else if (ann.kind == annotation_t::glossary_term)
				glossary.push_back(&ann);
		}

		if (!hyperlinks.empty())
		{
			std::sort(
			    hyperlinks.begin(),
			    hyperlinks.end(),
			    [](const annotation_t * a, const annotation_t * b)
			{
				if (a->source != b->source)
					return a->source < b->source;
				return a->old_text < b->old_text;
			});

			ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.8f, 1.0f), "Hyperlinks");
			std::set<std::string> seen_hyperlinks;
			int hi = 0;
			for (const auto * ann : hyperlinks)
			{
				std::string key = ann->old_text + "|" + ann->new_text;
				if (!seen_hyperlinks.insert(key).second)
					continue;
				ImGui::PushID(hi++);
				std::string label = "[" + ann->source + "] " + ann->old_text + " -> " + ann->new_text;
				if (ImGui::Selectable(label.c_str()))
					ImGui::SetClipboardText(ann->new_text.c_str());
				ImGui::PopID();
			}
			ImGui::Separator();
		}

		if (!glossary.empty())
		{
			std::sort(
			    glossary.begin(),
			    glossary.end(),
			    [](const annotation_t * a, const annotation_t * b)
			{
				if (a->source != b->source)
					return a->source < b->source;
				return a->old_text < b->old_text;
			});

			ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.4f, 1.0f), "Glossary");
			std::set<std::string> seen_glossary;
			int gi = 0;
			for (const auto * ann : glossary)
			{
				std::string key = ann->old_text + "|" + ann->new_text;
				if (!seen_glossary.insert(key).second)
					continue;
				ImGui::PushID(1000 + gi++);
				std::string label = "[" + ann->source + "] " + ann->old_text + " -> " + ann->new_text;
				if (ImGui::Selectable(label.c_str()))
					ImGui::SetClipboardText(ann->new_text.c_str());
				ImGui::PopID();
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

	auto * slot = workspace_.get_active_slot();
	if (!slot)
		return;

	const auto & active_dict = slot->data;

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

	bool is_editable = workspace_.is_user_slot(workspace_.get_active_index());

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
				history_.revert(*slot, row.type, entry.key_text, i);
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
	auto * speaker_slot = workspace_.get_active_slot();
	if (!speaker_slot)
	{
		ImGui::TextDisabled("No info records loaded");
		return;
	}

	const auto & dict = speaker_slot->data;
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

		visible_rows.push_back({ &entry.speaker, &entry.speaker_name, &entry.gender });
	}

	ImGuiTableSortSpecs * sort_specs = ImGui::TableGetSortSpecs();
	if (sort_specs != nullptr && sort_specs->SpecsDirty && sort_specs->SpecsCount > 0)
	{
		const auto & spec = sort_specs->Specs[0];
		bool ascending = (spec.SortDirection == ImGuiSortDirection_Ascending);
		int col = spec.ColumnIndex;

		std::sort(
		    visible_rows.begin(),
		    visible_rows.end(),
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

	auto * ann_slot = workspace_.get_active_slot();
	if (!ann_slot)
	{
		ImGui::EndChild();
		return;
	}

	const auto & active_dict = ann_slot->data;

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

	static const tools_t::dict_t empty_base_dict;
	const auto & base_dict = empty_base_dict;
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

	auto * hist_slot = workspace_.get_active_slot();
	if (!hist_slot)
	{
		ImGui::EndChild();
		return;
	}

	const auto & active_dict = hist_slot->data;

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
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				history_.revert(*hist_slot, row.type, entry.key_text, i);
				rebuild_row_data();
			}
		}

		ImGui::SameLine();
		if (ImGui::SmallButton("Revert"))
		{
			history_.revert(*hist_slot, row.type, entry.key_text, i);
			rebuild_row_data();
		}

		ImGui::PopID();
	}

	ImGui::EndChild();
}

void editor_app_t::render_dialogs()
{
	if (show_quit_dialog_)
		render_quit_dialog();
	if (show_unload_confirm_)
		render_unload_confirm_dialog();
}

void editor_app_t::render_quit_dialog()
{
	ImGui::OpenPopup("Unsaved Changes");

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	ImGui::Text("You have unsaved changes. What would you like to do?");
	ImGui::Spacing();

	for (int i = 0; i < workspace_.slot_count(); ++i)
	{
		if (!workspace_.is_user_slot(i))
			continue;
		const auto * slot = workspace_.get_slot(i);
		if (!slot || !slot->dirty)
			continue;
		auto pos = slot->path.find_last_of("\\/");
		std::string filename = (pos != std::string::npos) ? slot->path.substr(pos + 1) : slot->path;
		ImGui::BulletText("%s", filename.c_str());
	}

	ImGui::Separator();

	if (ImGui::Button("Save", ImVec2(120, 0)))
	{
		commit_richedit_text();
		save_all_encoded();
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

void editor_app_t::render_unload_confirm_dialog()
{
	ImGui::OpenPopup("Confirm Unload");

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (!ImGui::BeginPopupModal("Confirm Unload", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return;

	ImGui::Text("Dictionary has unsaved changes. Unload anyway?");
	ImGui::Separator();

	if (ImGui::Button("Yes", ImVec2(120, 0)))
	{
		if (pending_unload_index_ >= 0)
		{
			workspace_.unload_dict(pending_unload_index_);
			rebuild_row_data();
		}
		pending_unload_index_ = -1;
		show_unload_confirm_ = false;
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();

	if (ImGui::Button("No", ImVec2(120, 0)))
	{
		pending_unload_index_ = -1;
		show_unload_confirm_ = false;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
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

void editor_app_t::create_richedit(HWND parent)
{
	LoadLibraryA("Msftedit.dll");

	richedit_hwnd_ = CreateWindowExA(
	    0,
	    "RICHEDIT50W",
	    "",
	    WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | ES_NOHIDESEL,
	    0,
	    0,
	    100,
	    100,
	    parent,
	    nullptr,
	    GetModuleHandle(nullptr),
	    nullptr);

	SendMessageA(richedit_hwnd_, EM_SETTARGETDEVICE, 0, 0);
	SendMessageA(richedit_hwnd_, EM_SETBKGNDCOLOR, 0, RGB(255, 255, 255));
	SendMessageA(richedit_hwnd_, EM_SETEVENTMASK, 0, ENM_CHANGE);
	SendMessageA(richedit_hwnd_, EM_SETLIMITTEXT, 0x100000, 0);

	CHARFORMATA cf = {};
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_FACE | CFM_SIZE | CFM_COLOR;
	cf.yHeight = 200;
	cf.crTextColor = RGB(0, 0, 0);
	strcpy_s(cf.szFaceName, "Consolas");
	SendMessageA(richedit_hwnd_, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
}

void editor_app_t::position_richedit(float screen_x, float screen_y, float width, float height)
{
	if (!richedit_hwnd_)
		return;

	float adjusted_width = width;

	bool should_show = (adjusted_width > 10.0f && height > 10.0f);

	if (should_show)
	{
		SetWindowPos(
		    richedit_hwnd_,
		    HWND_TOP,
		    static_cast<int>(screen_x),
		    static_cast<int>(screen_y),
		    static_cast<int>(adjusted_width),
		    static_cast<int>(height),
		    SWP_NOACTIVATE | SWP_SHOWWINDOW);
		richedit_visible_ = true;
	}
	else if (richedit_visible_)
	{
		ShowWindow(richedit_hwnd_, SW_HIDE);
		richedit_visible_ = false;
	}
}

void editor_app_t::set_richedit_text(const std::string & text)
{
	if (!richedit_hwnd_)
		return;

	int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
	std::wstring wtext(wlen, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wtext.data(), wlen);
	SetWindowTextW(richedit_hwnd_, wtext.c_str());
}

std::string editor_app_t::get_richedit_text() const
{
	if (!richedit_hwnd_)
		return {};

	int wlen = GetWindowTextLengthW(richedit_hwnd_);
	if (wlen <= 0)
		return {};

	std::wstring wtext(wlen, L'\0');
	GetWindowTextW(richedit_hwnd_, wtext.data(), wlen + 1);

	int len = WideCharToMultiByte(CP_UTF8, 0, wtext.c_str(), wlen, nullptr, 0, nullptr, nullptr);
	std::string result(len, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wtext.c_str(), wlen, result.data(), len, nullptr, nullptr);
	return result;
}

void editor_app_t::commit_richedit_text()
{
	auto * slot = workspace_.get_active_slot();
	if (!slot)
		return;

	if (!workspace_.is_user_slot(workspace_.get_active_index()))
		return;

	auto prev_it = slot->data.find(editing_type_);
	if (prev_it == slot->data.end() || editing_record_index_ >= prev_it->second.records.size())
		return;

	std::string new_text = get_richedit_text();
	auto & prev_entry = prev_it->second.records[editing_record_index_];
	if (new_text == prev_entry.new_text)
		return;

	history_.record_change(editing_type_, prev_entry.key_text, prev_entry.new_text, new_text);
	prev_entry.new_text = new_text;
	prev_entry.status = tools_t::status_t::in_progress;
	auto vr = validation_.validate(editing_type_, new_text);
	if (vr.level == validation_level_t::error)
		prev_entry.status = tools_t::status_t::has_errors;
	slot->dirty = true;
	slot->modified_records.insert({ editing_type_, editing_record_index_ });
}

void editor_app_t::highlight_richedit_hyperlinks(
    const std::string & text,
    tools_t::rec_type_t type,
    const std::string & original_text)
{
	if (!richedit_hwnd_)
		return;

	if (type != tools_t::rec_type_t::info)
		return;

	auto original_annotations = annotations_mgr_.annotate(original_text, type);
	if (original_annotations.empty())
		return;

	std::set<std::string> new_texts_to_highlight;
	for (const auto & ann : original_annotations)
	{
		if (ann.kind != annotation_t::dial_topic)
			continue;
		if (ann.new_text.empty())
			continue;
		std::string new_lower = ann.new_text;
		for (auto & c : new_lower)
			c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
		new_texts_to_highlight.insert(new_lower);
	}

	if (new_texts_to_highlight.empty())
		return;

	std::string clean = text;
	for (size_t i = 0; i < clean.size();)
	{
		if (clean[i] == '\r' && i + 1 < clean.size() && clean[i + 1] == '\n')
		{
			clean.erase(i, 1);
			continue;
		}
		++i;
	}

	std::string clean_lower = clean;
	for (auto & c : clean_lower)
		c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

	std::vector<int> byte_to_wchar(clean.size() + 1, 0);
	int wchar_pos = 0;
	for (size_t i = 0; i < clean.size();)
	{
		byte_to_wchar[i] = wchar_pos;
		unsigned char c = static_cast<unsigned char>(clean[i]);
		int char_len = 1;
		if (c >= 0xF0)
			char_len = 4;
		else if (c >= 0xE0)
			char_len = 3;
		else if (c >= 0xC0)
			char_len = 2;

		unsigned int codepoint = 0;
		if (char_len == 1)
			codepoint = c;
		else if (char_len == 2)
			codepoint = c & 0x1F;
		else if (char_len == 3)
			codepoint = c & 0x0F;
		else
			codepoint = c & 0x07;

		for (int j = 1; j < char_len && i + j < clean.size(); ++j)
		{
			byte_to_wchar[i + j] = wchar_pos;
			codepoint = (codepoint << 6) | (static_cast<unsigned char>(clean[i + j]) & 0x3F);
		}

		wchar_pos += (codepoint > 0xFFFF) ? 2 : 1;
		i += char_len;
	}
	byte_to_wchar[clean.size()] = wchar_pos;

	SendMessageA(richedit_hwnd_, WM_SETREDRAW, FALSE, 0);

	CHARRANGE orig_sel;
	SendMessageA(richedit_hwnd_, EM_EXGETSEL, 0, (LPARAM)&orig_sel);

	for (const auto & new_lower : new_texts_to_highlight)
	{
		size_t pos = 0;
		while ((pos = clean_lower.find(new_lower, pos)) != std::string::npos)
		{
			size_t end_pos = pos + new_lower.size();

			CHARRANGE range;
			range.cpMin = byte_to_wchar[pos];
			range.cpMax = byte_to_wchar[end_pos];
			SendMessageA(richedit_hwnd_, EM_EXSETSEL, 0, (LPARAM)&range);

			CHARFORMAT2A cf = {};
			cf.cbSize = sizeof(cf);
			cf.dwMask = CFM_BACKCOLOR;
			cf.crBackColor = RGB(200, 220, 255);
			SendMessageA(richedit_hwnd_, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);

			pos = end_pos;
		}
	}

	SendMessageA(richedit_hwnd_, EM_EXSETSEL, 0, (LPARAM)&orig_sel);
	SendMessageA(richedit_hwnd_, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(richedit_hwnd_, nullptr, TRUE);
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
	constexpr float min_bottom = 50.0f;
	constexpr float min_main = 30.0f;

	float max_bottom = available_height - min_main;
	return std::clamp(requested, min_bottom, max_bottom);
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
	auto * slot = workspace_.get_active_slot();
	if (!slot)
		return;

	codepage_t cp = active_codepage();
	auto & dict = slot->data;
	for (auto & [type, chapter] : dict)
	{
		for (auto & entry : chapter.records)
		{
			entry.key_text = encode_from_utf8(entry.key_text, cp);
			entry.old_text = encode_from_utf8(entry.old_text, cp);
			entry.new_text = encode_from_utf8(entry.new_text, cp);
		}
	}

	workspace_.save_dict(workspace_.get_active_index());

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
	auto * slot = workspace_.get_active_slot();
	if (!slot)
		return;

	codepage_t cp = active_codepage();
	auto & dict = slot->data;
	for (auto & [type, chapter] : dict)
	{
		for (auto & entry : chapter.records)
		{
			entry.key_text = encode_from_utf8(entry.key_text, cp);
			entry.old_text = encode_from_utf8(entry.old_text, cp);
			entry.new_text = encode_from_utf8(entry.new_text, cp);
		}
	}

	workspace_.save_dict_as(workspace_.get_active_index(), path);

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

void editor_app_t::save_slot_encoded(int slot_index)
{
	auto * slot = workspace_.get_slot(slot_index);
	if (!slot)
		return;

	codepage_t cp = active_codepage();
	auto & dict = slot->data;
	for (auto & [type, chapter] : dict)
	{
		for (auto & entry : chapter.records)
		{
			entry.key_text = encode_from_utf8(entry.key_text, cp);
			entry.old_text = encode_from_utf8(entry.old_text, cp);
			entry.new_text = encode_from_utf8(entry.new_text, cp);
		}
	}

	workspace_.save_dict(slot_index);

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

void editor_app_t::save_slot_as_encoded(int slot_index, const std::string & path)
{
	auto * slot = workspace_.get_slot(slot_index);
	if (!slot)
		return;

	codepage_t cp = active_codepage();
	auto & dict = slot->data;
	for (auto & [type, chapter] : dict)
	{
		for (auto & entry : chapter.records)
		{
			entry.key_text = encode_from_utf8(entry.key_text, cp);
			entry.old_text = encode_from_utf8(entry.old_text, cp);
			entry.new_text = encode_from_utf8(entry.new_text, cp);
		}
	}

	workspace_.save_dict_as(slot_index, path);

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

void editor_app_t::save_all_encoded()
{
	codepage_t cp = active_codepage();
	for (int i = 0; i < workspace_.slot_count(); ++i)
	{
		if (!workspace_.is_user_slot(i))
			continue;
		auto * slot = workspace_.get_slot(i);
		if (!slot || !slot->dirty)
			continue;

		auto & dict = slot->data;
		for (auto & [type, chapter] : dict)
		{
			for (auto & entry : chapter.records)
			{
				entry.key_text = encode_from_utf8(entry.key_text, cp);
				entry.old_text = encode_from_utf8(entry.old_text, cp);
				entry.new_text = encode_from_utf8(entry.new_text, cp);
			}
		}

		workspace_.save_dict(i);

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

	static const tools_t::dict_t empty_base_dict;
	auto it = empty_base_dict.find(type);
	if (it == empty_base_dict.end())
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
