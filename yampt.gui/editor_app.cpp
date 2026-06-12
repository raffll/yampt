#define NOMINMAX

#include "editor_app.hpp"
#include "encoding_utils.hpp"
#include "imgui.h"
#include "status_colors.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>

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

	if (config_.window_x >= 0 && config_.window_y >= 0)
	{
		SDL_RestoreWindow(window_);
		SDL_SetWindowPosition(window_, config_.window_x, config_.window_y);
		SDL_SetWindowSize(window_, config_.window_w, config_.window_h);
	}

	sidebar_width_ = config_.sidebar_width;
	bottom_height_ = config_.bottom_height;
	sidebar_visible_ = config_.sidebar_visible;
	bottom_visible_ = config_.bottom_visible;
	encoding_index_ = config_.encoding_index;
	validation_.set_codepage(supported_codepages[encoding_index_]);

	for (const auto & path : config_.user_dict_paths)
	{
		int prev_count = workspace_.slot_count();
		int idx = workspace_.load_dict(path, dict_kind_t::user);
		if (idx < 0)
			tools_t::add_log("[warn] cannot load user dict: \"" + path + "\"\r\n");
		else if (workspace_.slot_count() > prev_count)
			decode_dict_from_codepage(workspace_.get_slot(idx)->data, active_codepage());
	}

	for (const auto & path : config_.base_dict_paths)
	{
		int prev_count = workspace_.slot_count();
		int idx = workspace_.load_dict(path, dict_kind_t::base);
		if (idx < 0)
			tools_t::add_log("[warn] cannot load base dict: \"" + path + "\"\r\n");
		else if (workspace_.slot_count() > prev_count)
			decode_dict_from_codepage(workspace_.get_slot(idx)->data, active_codepage());
	}

	if (config_.active_dict_index >= 0 && config_.active_dict_index < workspace_.slot_count())
		workspace_.set_active(config_.active_dict_index);

	{
		auto dict_dir = get_exe_directory() + "\\dictionaries";
		if (std::filesystem::is_directory(dict_dir))
		{
			for (const auto & entry : std::filesystem::directory_iterator(dict_dir))
			{
				if (!entry.is_regular_file())
					continue;
				auto path = entry.path();
				if (path.extension() != ".aff")
					continue;
				auto dic_path = path;
				dic_path.replace_extension(".dic");
				if (!std::filesystem::exists(dic_path))
					continue;
				auto stem = path.stem().string();
				spell_langs_.push_back({ stem, path.string(), dic_path.string() });
			}
		}

		spell_lang_index_ = config_.spell_lang_index;
	}

	SDL_SysWMinfo wm_info = {};
	SDL_VERSION(&wm_info.version);
	SDL_GetWindowWMInfo(window_, &wm_info);
	create_richedit(wm_info.info.win.window);

	rebuild_annotations();
	rebuild_row_data();

	if (spell_lang_index_ >= 0 && spell_lang_index_ < static_cast<int>(spell_langs_.size()))
		spell_checker_.load(spell_langs_[spell_lang_index_].aff_path, spell_langs_[spell_lang_index_].dic_path);
}

void editor_app_t::frame()
{
	if (SDL_GetWindowFlags(window_) & SDL_WINDOW_MINIMIZED)
	{
		if (richedit_visible_ && richedit_hwnd_)
		{
			ShowWindow(richedit_hwnd_, SW_HIDE);
			richedit_visible_ = false;
		}
		if (richedit_original_visible_ && richedit_original_hwnd_)
		{
			ShowWindow(richedit_original_hwnd_, SW_HIDE);
			richedit_original_visible_ = false;
		}
		return;
	}

	if (richedit_hwnd_ && GetFocus() == richedit_hwnd_)
	{
		if (ImGui::GetIO().WantTextInput)
			SetFocus(GetParent(richedit_hwnd_));
	}

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
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 1.0f));
	render_toolbar();
	ImGui::Spacing();
	render_status_summary_bar();
	render_dial_type_bar();
	ImGui::PopStyleVar();

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
		ImGui::BeginChild("SidebarArea", ImVec2(sidebar_width_, 0), ImGuiChildFlags_None,
		    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		float sidebar_height = ImGui::GetContentRegionAvail().y;
		float sidebar_top_height = sidebar_height;
		if (bottom_visible_)
		{
			float splitter_h = 6.0f;
			sidebar_top_height -= (info_height_ + splitter_h);
			if (sidebar_top_height < 50.0f)
				sidebar_top_height = 50.0f;
		}

		ImGui::BeginChild("SidebarTop", ImVec2(0, sidebar_top_height), ImGuiChildFlags_None,
		    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		render_sidebar();
		ImGui::EndChild();

		if (bottom_visible_)
		{
			float max_h = clamp_bottom_height(sidebar_height, sidebar_height);
			ImGui::PushID("sidebar_bottom_splitter");
			render_splitter_horizontal(info_height_, 50.0f, max_h);
			ImGui::PopID();
			render_info_panel();
		}

		ImGui::EndChild();

		float max_w = clamp_sidebar_width(window_width, window_width);
		render_splitter_vertical(sidebar_width_, 150.0f, max_w);
	}

	ImGui::BeginGroup();

	float right_height = ImGui::GetContentRegionAvail().y;
	float splitter_h = 6.0f;
	float editor_h = bottom_height_;
	float main_panel_height = right_height - editor_h - splitter_h;
	if (main_panel_height < 50.0f)
		main_panel_height = 50.0f;

	ImGui::BeginChild(
	    "MainPanelArea",
	    ImVec2(0, main_panel_height),
	    ImGuiChildFlags_None,
	    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	render_main_panel();
	ImGui::EndChild();

	{
		float max_h = clamp_bottom_height(right_height, right_height);
		ImGui::PushID("editor_splitter");
		render_splitter_horizontal(bottom_height_, 50.0f, max_h);
		ImGui::PopID();
	}

	ImGui::BeginChild("EditorPanel", ImVec2(0, 0), ImGuiChildFlags_Borders);
	if (selected_row_ < 0 || selected_row_ >= static_cast<int>(left_rows_.size()))
	{
		ImGui::TextDisabled("No record selected");
		if (richedit_visible_ && richedit_hwnd_)
		{
			ShowWindow(richedit_hwnd_, SW_HIDE);
			richedit_visible_ = false;
		}
		if (richedit_original_visible_ && richedit_original_hwnd_)
		{
			ShowWindow(richedit_original_hwnd_, SW_HIDE);
			richedit_original_visible_ = false;
		}
	}
	else
	{
		render_editor_tab();
	}
	ImGui::EndChild();

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

	if (richedit_original_hwnd_)
	{
		DestroyWindow(richedit_original_hwnd_);
		richedit_original_hwnd_ = nullptr;
	}

	config_.split_ratio = split_ratio_;
	config_.sidebar_width = sidebar_width_;
	config_.bottom_height = bottom_height_;
	config_.sidebar_visible = sidebar_visible_;
	config_.bottom_visible = bottom_visible_;
	config_.encoding_index = encoding_index_;
	config_.spell_lang_index = spell_lang_index_;

	int wx, wy, ww, wh;
	SDL_GetWindowPosition(window_, &wx, &wy);
	SDL_GetWindowSize(window_, &ww, &wh);
	config_.window_x = wx;
	config_.window_y = wy;
	config_.window_w = ww;
	config_.window_h = wh;
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
		std::string name = workspace_.is_user_slot(i) ? "user" : "base";
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
	std::string query(search_buffer_.data());
	std::string query_lc = query;
	if (!search_case_sensitive_)
		std::transform(query_lc.begin(), query_lc.end(), query_lc.begin(), ::tolower);

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

			if (type_filter_solo_ &&
			    (type == tools_t::rec_type_t::info || type == tools_t::rec_type_t::bnam) &&
			    dial_type_filter_.size() < 5)
			{
				const auto & key = chapter.records[i].key_text;
				if (!key.empty() && dial_type_filter_.count(key[0]) == 0)
					continue;
			}

			if (type_filter_solo_ && type == tools_t::rec_type_t::fnam && fnam_type_filter_.size() < 24)
			{
				const auto & key = chapter.records[i].key_text;
				auto caret = key.find('^');
				if (caret != std::string::npos)
				{
					auto prefix = key.substr(0, caret);
					if (fnam_type_filter_.count(prefix) == 0)
						continue;
				}
			}

			if (type_filter_solo_ && type == tools_t::rec_type_t::desc && desc_type_filter_.size() < 3)
			{
				const auto & key = chapter.records[i].key_text;
				auto caret = key.find('^');
				if (caret != std::string::npos)
				{
					auto prefix = key.substr(0, caret);
					if (desc_type_filter_.count(prefix) == 0)
						continue;
				}
			}

			if (type_filter_solo_ && type == tools_t::rec_type_t::indx && indx_type_filter_.size() < 2)
			{
				const auto & key = chapter.records[i].key_text;
				auto caret = key.find('^');
				if (caret != std::string::npos)
				{
					auto prefix = key.substr(0, caret);
					if (indx_type_filter_.count(prefix) == 0)
						continue;
				}
			}

			if (!query.empty())
			{
				const auto & rec = chapter.records[i];
				bool found = false;

				if (search_case_sensitive_)
				{
					found = rec.key_text.find(query) != std::string::npos ||
					        rec.old_text.find(query) != std::string::npos ||
					        rec.new_text.find(query) != std::string::npos;
				}
				else
				{
					std::string key_lc = rec.key_text;
					std::string old_lc = rec.old_text;
					std::string new_lc = rec.new_text;
					std::transform(key_lc.begin(), key_lc.end(), key_lc.begin(), ::tolower);
					std::transform(old_lc.begin(), old_lc.end(), old_lc.begin(), ::tolower);
					std::transform(new_lc.begin(), new_lc.end(), new_lc.begin(), ::tolower);
					found = key_lc.find(query_lc) != std::string::npos ||
					        old_lc.find(query_lc) != std::string::npos ||
					        new_lc.find(query_lc) != std::string::npos;
				}

				if (!found)
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

	float toolbar_height = 48.0f;
	ImGui::BeginChild("Toolbar", ImVec2(0, toolbar_height), ImGuiChildFlags_None);

	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Search:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(200.0f);
	if (ImGui::InputText("##search", search_buffer_.data(), search_buffer_.size()))
	{
		rebuild_row_data();
	}

	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Case:");
	ImGui::SameLine();
	if (ImGui::Checkbox("##case", &search_case_sensitive_))
	{
		rebuild_row_data();
	}

	ImGui::SameLine(0, 8.0f);
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted("Encoding:");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(250.0f);
	const char * encoding_preview = codepage_name(supported_codepages[encoding_index_]);
	if (ImGui::BeginCombo("##encoding", encoding_preview))
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
					validation_.set_codepage(new_cp);
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

	if (!spell_langs_.empty())
	{
		ImGui::SameLine(0, 8.0f);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Spelling:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.0f);
		const char * spell_preview = (spell_lang_index_ >= 0 && spell_lang_index_ < static_cast<int>(spell_langs_.size()))
		                                 ? spell_langs_[spell_lang_index_].name.c_str()
		                                 : "None";
		if (ImGui::BeginCombo("##spelling", spell_preview))
		{
			if (ImGui::Selectable("None", spell_lang_index_ < 0))
			{
				spell_lang_index_ = -1;
				spell_checker_ = spell_checker_t();
				config_.spell_lang_index = -1;
				config_.save(config_path_);
				if (selected_row_ >= 0 && selected_row_ < static_cast<int>(left_rows_.size()))
				{
					const auto & row = left_rows_[selected_row_];
					auto * sl = workspace_.get_active_slot();
					if (sl)
					{
						auto it = sl->data.find(row.type);
						if (it != sl->data.end() && row.record_index < it->second.records.size())
						{
							const auto & entry = it->second.records[row.record_index];
							richedit_ignore_change_ = true;
							set_richedit_text(entry.new_text);
							if (row.type == tools_t::rec_type_t::info)
								highlight_richedit_hyperlinks(entry.new_text, row.type, entry.old_text);
							else if (row.type == tools_t::rec_type_t::sctx || row.type == tools_t::rec_type_t::bnam ||
							         row.type == tools_t::rec_type_t::text)
								highlight_richedit_syntax(entry.new_text, row.type);
							richedit_ignore_change_ = false;
						}
					}
				}
			}
			for (int si = 0; si < static_cast<int>(spell_langs_.size()); ++si)
			{
				bool is_selected = (si == spell_lang_index_);
				if (ImGui::Selectable(spell_langs_[si].name.c_str(), is_selected))
				{
					spell_lang_index_ = si;
					spell_checker_.load(spell_langs_[si].aff_path, spell_langs_[si].dic_path);
					config_.spell_lang_index = si;
					config_.save(config_path_);
					if (selected_row_ >= 0 && selected_row_ < static_cast<int>(left_rows_.size()))
					{
						const auto & row = left_rows_[selected_row_];
						auto * sl = workspace_.get_active_slot();
						if (sl)
						{
							auto it = sl->data.find(row.type);
							if (it != sl->data.end() && row.record_index < it->second.records.size())
							{
								const auto & entry = it->second.records[row.record_index];
								richedit_ignore_change_ = true;
								set_richedit_text(entry.new_text);
								if (row.type == tools_t::rec_type_t::info)
									highlight_richedit_hyperlinks(entry.new_text, row.type, entry.old_text);
								else if (row.type == tools_t::rec_type_t::sctx || row.type == tools_t::rec_type_t::bnam ||
								         row.type == tools_t::rec_type_t::text)
									highlight_richedit_syntax(entry.new_text, row.type);
								highlight_richedit_spelling(entry.new_text);
								richedit_ignore_change_ = false;
							}
						}
					}
				}
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
	}

	ImGui::Dummy(ImVec2(0, 4.0f));

	{
		bool all_active = (type_filter_.size() == std::size(filter_types));
		if (all_active)
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
		ImGui::SmallButton("All");
		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(5);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			if (all_active)
			{
				type_filter_.clear();
			}
			else
			{
				type_filter_.clear();
				for (auto t : filter_types)
					type_filter_.insert(t);
			}
			type_filter_solo_ = false;
			rebuild_row_data();
		}
	}

	for (size_t i = 0; i < std::size(filter_types); ++i)
	{
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

		std::string label = std::string(get_type_display_name(type)) + " (" + std::to_string(count) + ")";

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
					if (f5_row.type == tools_t::rec_type_t::info)
						highlight_richedit_hyperlinks(current_text, f5_row.type, f5_entry.old_text);
					else if (f5_row.type == tools_t::rec_type_t::sctx || f5_row.type == tools_t::rec_type_t::bnam ||
					         f5_row.type == tools_t::rec_type_t::text)
						highlight_richedit_syntax(current_text, f5_row.type);

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

	ImGui::BeginChild("Sidebar", ImVec2(0, 0), ImGuiChildFlags_Borders);

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
			if (editing_row_ >= 0)
				commit_richedit_text();
			editing_row_ = -1;
			editing_type_ = tools_t::rec_type_t::unknown;
			editing_record_index_ = 0;
			selected_row_ = -1;
			selected_row_left_ = -1;

			auto * old_slot = workspace_.get_active_slot();
			if (old_slot)
			{
				old_slot->filters.type_filter = type_filter_;
				old_slot->filters.status_filter = status_filter_;
				old_slot->filters.type_filter_solo = type_filter_solo_;
			}

			workspace_.set_active(i);

			auto * new_slot = workspace_.get_active_slot();
			if (new_slot)
			{
				type_filter_ = new_slot->filters.type_filter;
				status_filter_ = new_slot->filters.status_filter;
				type_filter_solo_ = new_slot->filters.type_filter_solo;
			}

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
			if (editing_row_ >= 0)
				commit_richedit_text();
			editing_row_ = -1;
			editing_type_ = tools_t::rec_type_t::unknown;
			editing_record_index_ = 0;
			selected_row_ = -1;
			selected_row_left_ = -1;

			auto * old_slot = workspace_.get_active_slot();
			if (old_slot)
			{
				old_slot->filters.type_filter = type_filter_;
				old_slot->filters.status_filter = status_filter_;
				old_slot->filters.type_filter_solo = type_filter_solo_;
			}

			workspace_.set_active(i);

			auto * new_slot = workspace_.get_active_slot();
			if (new_slot)
			{
				type_filter_ = new_slot->filters.type_filter;
				status_filter_ = new_slot->filters.status_filter;
				type_filter_solo_ = new_slot->filters.type_filter_solo;
			}

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
	static const char * status_names[] = { "untranslated", "missing", "duplicate", "coords",     "fingerprint",
		                                   "heuristic",    "info",    "exact",     "wilderness", "region",
		                                   "matched",      "error",   "identical", "translated", "reused",
		                                   "adapted",      "changed", "in_progress" };
	static const char * status_labels[] = { "Untranslated", "Missing", "Duplicate", "Coords",      "Fingerprint",
		                                    "Heuristic",    "Info",    "Exact",     "Wilderness",  "Region",
		                                    "Matched",      "Error",   "Identical", "Translated",  "Reused",
		                                    "Adapted",      "Changed", "In Progress" };
	static constexpr size_t status_count = 18;

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

	ImGui::BeginChild("StatusSummary", ImVec2(0, 20), ImGuiChildFlags_None);

	{
		bool all_statuses_active = status_filter_.empty();
		if (!all_statuses_active)
		{
			bool all_visible_selected = true;
			for (size_t i = 0; i < status_count; ++i)
			{
				if (counts[i] > 0 && status_filter_.count(status_names[i]) == 0)
				{
					all_visible_selected = false;
					break;
				}
			}
			if (all_visible_selected && !status_filter_.empty())
			{
				status_filter_.clear();
				all_statuses_active = true;
			}
		}
		if (all_statuses_active)
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
		ImGui::SmallButton("All");
		ImGui::PopStyleVar(1);
		ImGui::PopStyleColor(5);
		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			if (all_statuses_active)
			{
				status_filter_.insert("__none__");
			}
			else
			{
				status_filter_.clear();
			}
			status_filter_solo_ = false;
			rebuild_row_data();
		}
	}

	for (size_t i = 0; i < status_count; ++i)
	{
		if (counts[i] == 0)
			continue;

		ImGui::SameLine();

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

	ImGui::SameLine();
	std::string total = "Total: " + std::to_string(static_cast<int>(left_rows_.size()));
	ImGui::TextUnformatted(total.c_str());

	ImGui::EndChild();
}

void editor_app_t::render_dial_type_bar()
{
	bool any_dial = type_filter_solo_ &&
	               (type_filter_.count(tools_t::rec_type_t::info) > 0 ||
	                type_filter_.count(tools_t::rec_type_t::bnam) > 0);
	bool any_desc = type_filter_solo_ && type_filter_.count(tools_t::rec_type_t::desc) > 0;
	bool any_indx = type_filter_solo_ && type_filter_.count(tools_t::rec_type_t::indx) > 0;
	bool any_fnam = type_filter_solo_ && type_filter_.count(tools_t::rec_type_t::fnam) > 0;

	static const char dial_types[] = { 'T', 'V', 'G', 'P', 'J' };
	static const char * dial_labels[] = { "Topics", "Voice", "Greetings", "Persuasion", "Journal" };
	static const char * desc_types[] = { "BSGN", "CLAS", "RACE" };
	static const char * desc_labels[] = { "Birthsigns", "Classes", "Races" };
	static const char * indx_types[] = { "SKIL", "MGEF" };
	static const char * indx_labels[] = { "Skills", "Magic Effects" };

	ImGui::BeginChild("DialTypeBar", ImVec2(0, 20), ImGuiChildFlags_None);

	if (any_dial)
	{
		for (int i = 0; i < 5; ++i)
		{
			if (i > 0)
				ImGui::SameLine();
			bool is_active = dial_type_filter_.count(dial_types[i]) > 0;

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
			ImGui::SmallButton(dial_labels[i]);
			ImGui::PopStyleVar(1);
			ImGui::PopStyleColor(5);

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				if (dial_type_filter_.size() == 1 && dial_type_filter_.count(dial_types[i]) > 0)
					dial_type_filter_ = { 'T', 'V', 'G', 'P', 'J' };
				else
				{
					dial_type_filter_.clear();
					dial_type_filter_.insert(dial_types[i]);
				}
				rebuild_row_data();
			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				if (is_active)
					dial_type_filter_.erase(dial_types[i]);
				else
					dial_type_filter_.insert(dial_types[i]);
				rebuild_row_data();
			}
		}
	}

	if (any_desc)
	{
		for (int i = 0; i < 3; ++i)
		{
			if (i > 0)
				ImGui::SameLine();
			bool is_active = desc_type_filter_.count(desc_types[i]) > 0;

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
			ImGui::SmallButton(desc_labels[i]);
			ImGui::PopStyleVar(1);
			ImGui::PopStyleColor(5);

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				if (desc_type_filter_.size() == 1 && desc_type_filter_.count(desc_types[i]) > 0)
					desc_type_filter_ = { "BSGN", "CLAS", "RACE" };
				else
				{
					desc_type_filter_.clear();
					desc_type_filter_.insert(desc_types[i]);
				}
				rebuild_row_data();
			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				if (is_active)
					desc_type_filter_.erase(desc_types[i]);
				else
					desc_type_filter_.insert(desc_types[i]);
				rebuild_row_data();
			}
		}
	}

	if (any_indx)
	{
		for (int i = 0; i < 2; ++i)
		{
			if (i > 0)
				ImGui::SameLine();
			bool is_active = indx_type_filter_.count(indx_types[i]) > 0;

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
			ImGui::SmallButton(indx_labels[i]);
			ImGui::PopStyleVar(1);
			ImGui::PopStyleColor(5);

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				if (indx_type_filter_.size() == 1 && indx_type_filter_.count(indx_types[i]) > 0)
					indx_type_filter_ = { "SKIL", "MGEF" };
				else
				{
					indx_type_filter_.clear();
					indx_type_filter_.insert(indx_types[i]);
				}
				rebuild_row_data();
			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				if (is_active)
					indx_type_filter_.erase(indx_types[i]);
				else
					indx_type_filter_.insert(indx_types[i]);
				rebuild_row_data();
			}
		}
	}

	if (any_fnam)
	{
		static const char * fnam_types_arr[] = {
			"ACTI", "ALCH", "APPA", "ARMO", "BOOK", "BSGN", "CLAS", "CLOT", "CONT", "CREA", "DOOR", "FACT",
			"INGR", "LIGH", "LOCK", "MISC", "NPC_", "PROB", "RACE", "REGN", "REPA", "SPEL", "WEAP",
		};
		static const char * fnam_labels_arr[] = {
			"Activators", "Potions",   "Apparatus", "Armor",    "Books",   "Birthsigns", "Classes",   "Clothing",
			"Containers", "Creatures", "Doors",     "Factions", "Ingred.", "Lights",     "Lockpicks", "Misc",
			"NPCs",       "Probes",    "Races",     "Regions",  "Repairs", "Spells",     "Weapons",
		};
		static constexpr size_t fnam_count_arr = 23;

		for (size_t i = 0; i < fnam_count_arr; ++i)
		{
			if (i > 0)
				ImGui::SameLine();
			bool is_active = fnam_type_filter_.count(fnam_types_arr[i]) > 0;

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
			ImGui::SmallButton(fnam_labels_arr[i]);
			ImGui::PopStyleVar(1);
			ImGui::PopStyleColor(5);

			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				if (fnam_type_filter_.size() == 1 && fnam_type_filter_.count(fnam_types_arr[i]) > 0)
				{
					fnam_type_filter_.clear();
					for (size_t j = 0; j < fnam_count_arr; ++j)
						fnam_type_filter_.insert(fnam_types_arr[j]);
				}
				else
				{
					fnam_type_filter_.clear();
					fnam_type_filter_.insert(fnam_types_arr[i]);
				}
				rebuild_row_data();
			}

			if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
			{
				if (is_active)
					fnam_type_filter_.erase(fnam_types_arr[i]);
				else
					fnam_type_filter_.insert(fnam_types_arr[i]);
				rebuild_row_data();
			}
		}
	}

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

	if (!ImGui::BeginTable("##main_table", 5, flags))
	{
		ImGui::EndChild();
		return;
	}

	ImGui::TableSetupScrollFreeze(0, 1);
	ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_DefaultSort, 50.0f);
	ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, config_.column_widths[0]);
	ImGui::TableSetupColumn("Original", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Translation", ImGuiTableColumnFlags_WidthStretch);
	ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, config_.column_widths[3]);
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
			ImGui::PushID(i);

			if (ImGui::Selectable(
			        tools_t::type_to_str(row_ref.type).c_str(),
			        is_selected,
			        ImGuiSelectableFlags_SpanAllColumns,
			        ImVec2(0, 0)))
			{
				selected_row = i;
				selected_row_left_ = i;
			}

			if (is_editable && ImGui::BeginPopupContextItem("##row_ctx"))
			{
				auto vr = validation_.validate(row_ref.type, entry.new_text);
				bool limit_exceeded = (vr.level == validation_level_t::error);

				if (ImGui::BeginMenu("Set Status"))
				{
					static const char * ctx_status_names[] = { "untranslated", "translated", "error" };
					static const char * ctx_status_labels[] = { "Untranslated", "Translated", "Error" };

					for (int s = 0; s < 3; ++s)
					{
						bool disabled = limit_exceeded && std::string(ctx_status_names[s]) != "error";
						if (disabled)
							ImGui::BeginDisabled();

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

						if (disabled)
							ImGui::EndDisabled();
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

			const char * status_label = get_status_display_name(entry.status);
			ImGui::TextUnformatted(status_label);

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
		ImGui::TextWrapped("%s", text.c_str());
		return;
	}

	for (const auto & token : tokens)
	{
		if (token.start >= token.end || token.start >= text.size())
			continue;

		size_t end = std::min(token.end, text.size());
		const char * seg_start = text.c_str() + token.start;
		const char * seg_end = text.c_str() + end;

		bool need_same_line = (token.start > 0 && text[token.start - 1] != '\n');

		const char * p = seg_start;
		while (p < seg_end)
		{
			const char * nl = p;
			while (nl < seg_end && *nl != '\n')
				++nl;

			if (nl > p)
			{
				if (need_same_line)
					ImGui::SameLine(0, 0);

				switch (token.type)
				{
				case token_type_t::mwscript_function:
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
					ImGui::TextUnformatted(p, nl);
					ImGui::PopStyleColor();
					break;
				case token_type_t::mwscript_comment:
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
					ImGui::TextUnformatted(p, nl);
					ImGui::PopStyleColor();
					break;
				case token_type_t::mwscript_string:
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
					ImGui::TextUnformatted(p, nl);
					ImGui::PopStyleColor();
					break;
				case token_type_t::html_tag:
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.6f, 1.0f));
					ImGui::TextUnformatted(p, nl);
					ImGui::PopStyleColor();
					break;
				default:
					ImGui::TextUnformatted(p, nl);
					break;
				}
			}

			if (nl < seg_end && *nl == '\n')
			{
				ImGui::NewLine();
				need_same_line = false;
				p = nl + 1;
			}
			else
			{
				need_same_line = true;
				p = nl;
				break;
			}
		}
	}
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

	bool same_original = (editing_row_ == selected_row_ && editing_type_ == row.type &&
	                       editing_record_index_ == row.record_index);

	if (!same_original && richedit_original_hwnd_)
	{
		HWND prev_focus = GetFocus();
		int wlen_orig = MultiByteToWideChar(CP_UTF8, 0, entry.old_text.c_str(), static_cast<int>(entry.old_text.size()), nullptr, 0);
		std::wstring wtext_orig(wlen_orig, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, entry.old_text.c_str(), static_cast<int>(entry.old_text.size()), wtext_orig.data(), wlen_orig);
		SetWindowTextW(richedit_original_hwnd_, wtext_orig.c_str());
		if (prev_focus && prev_focus != richedit_original_hwnd_)
			SetFocus(prev_focus);

		if (row.type == tools_t::rec_type_t::sctx || row.type == tools_t::rec_type_t::bnam ||
		    row.type == tools_t::rec_type_t::text)
		{
			auto tokens = syntax_.tokenize(entry.old_text, row.type);
			if (!tokens.empty())
			{
				std::vector<int> b2w(entry.old_text.size() + 1, 0);
				int wp = 0;
				for (size_t bi = 0; bi < entry.old_text.size();)
				{
					b2w[bi] = wp;
					unsigned char c = static_cast<unsigned char>(entry.old_text[bi]);
					if (c == '\n' && bi > 0 && entry.old_text[bi - 1] == '\r') { bi++; continue; }
					int cl = 1;
					if (c >= 0xF0) cl = 4; else if (c >= 0xE0) cl = 3; else if (c >= 0xC0) cl = 2;
					unsigned int cp = (cl == 1) ? c : (cl == 2) ? (c & 0x1F) : (cl == 3) ? (c & 0x0F) : (c & 0x07);
					for (int j = 1; j < cl && bi + j < entry.old_text.size(); ++j)
					{ b2w[bi + j] = wp; cp = (cp << 6) | (static_cast<unsigned char>(entry.old_text[bi + j]) & 0x3F); }
					wp += (cp > 0xFFFF) ? 2 : 1;
					bi += cl;
				}
				b2w[entry.old_text.size()] = wp;

				SendMessageA(richedit_original_hwnd_, WM_SETREDRAW, FALSE, 0);
				CHARRANGE orig_sel_o;
				SendMessageA(richedit_original_hwnd_, EM_EXGETSEL, 0, (LPARAM)&orig_sel_o);
				for (const auto & token : tokens)
				{
					if (token.type == token_type_t::normal) continue;
					if (token.start >= token.end || token.start >= entry.old_text.size()) continue;
					size_t end = std::min(token.end, entry.old_text.size());
					COLORREF color = RGB(0, 0, 0);
					switch (token.type) {
					case token_type_t::mwscript_function: color = RGB(100, 180, 255); break;
					case token_type_t::mwscript_comment: color = RGB(128, 128, 128); break;
					case token_type_t::mwscript_string: color = RGB(200, 150, 50); break;
					case token_type_t::html_tag: color = RGB(140, 40, 50); break;
					default: continue;
					}
					CHARRANGE range;
					range.cpMin = b2w[token.start];
					range.cpMax = b2w[end];
					SendMessageA(richedit_original_hwnd_, EM_EXSETSEL, 0, (LPARAM)&range);
					CHARFORMAT2A cf2 = {};
					cf2.cbSize = sizeof(cf2);
					cf2.dwMask = CFM_COLOR;
					cf2.dwEffects = 0;
					cf2.crTextColor = color;
					SendMessageA(richedit_original_hwnd_, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf2);
				}
				SendMessageA(richedit_original_hwnd_, EM_EXSETSEL, 0, (LPARAM)&orig_sel_o);
				SendMessageA(richedit_original_hwnd_, WM_SETREDRAW, TRUE, 0);
				InvalidateRect(richedit_original_hwnd_, nullptr, TRUE);
			}
		}
	}

	ImVec2 orig_cursor = ImGui::GetCursorScreenPos();
	ImVec2 orig_region = ImGui::GetContentRegionAvail();

	if (richedit_original_hwnd_)
	{
		float ow = orig_region.x;
		float oh = orig_region.y;
		bool should_show_orig = (ow > 10.0f && oh > 20.0f);
		if (should_show_orig)
		{
			SetWindowPos(richedit_original_hwnd_, HWND_TOP,
			    static_cast<int>(orig_cursor.x), static_cast<int>(orig_cursor.y),
			    static_cast<int>(ow), static_cast<int>(oh),
			    SWP_NOACTIVATE | (richedit_original_visible_ ? 0 : SWP_SHOWWINDOW));
			if (!richedit_original_visible_)
				ShowWindow(richedit_original_hwnd_, SW_SHOWNOACTIVATE);
			richedit_original_visible_ = true;
		}
		else if (richedit_original_visible_)
		{
			ShowWindow(richedit_original_hwnd_, SW_HIDE);
			richedit_original_visible_ = false;
		}
	}

	ImGui::Dummy(orig_region);
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
		else if (row.type == tools_t::rec_type_t::sctx || row.type == tools_t::rec_type_t::bnam ||
		         row.type == tools_t::rec_type_t::text)
			highlight_richedit_syntax(entry.new_text, row.type);
		highlight_richedit_spelling(entry.new_text);
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

	if (row.type == tools_t::rec_type_t::fnam && !entry.enchantment.empty())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "Enchantment:");
		ImGui::SameLine();
		ImGui::TextUnformatted(entry.enchantment.c_str());
		ImGui::Separator();
	}

	if (row.type == tools_t::rec_type_t::info)
	{
		if (!entry.speaker_name.empty())
		{
			ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.9f, 1.0f), "Speaker:");
			ImGui::SameLine();
			std::string speaker_label = entry.speaker_name;
			if (!entry.gender.empty())
				speaker_label += " (" + entry.gender + ")";
			ImGui::TextUnformatted(speaker_label.c_str());
			ImGui::Separator();
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

	if (entry.status == tools_t::status_t::changed && base_it != base_dict.end())
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
	cf.yHeight = 160;
	cf.crTextColor = RGB(0, 0, 0);
	strcpy_s(cf.szFaceName, "Segoe UI");
	SendMessageA(richedit_hwnd_, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);

	PARAFORMAT2 pf = {};
	pf.cbSize = sizeof(pf);
	pf.dwMask = PFM_TABSTOPS;
	pf.cTabCount = 32;
	for (int t = 0; t < 32; ++t)
		pf.rgxTabs[t] = (t + 1) * 320;
	SendMessageA(richedit_hwnd_, EM_SETPARAFORMAT, 0, (LPARAM)&pf);

	richedit_original_hwnd_ = CreateWindowExA(
	    0,
	    "RICHEDIT50W",
	    "",
	    WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | ES_NOHIDESEL,
	    0,
	    0,
	    100,
	    100,
	    parent,
	    nullptr,
	    GetModuleHandle(nullptr),
	    nullptr);

	SendMessageA(richedit_original_hwnd_, EM_SETTARGETDEVICE, 0, 0);
	SendMessageA(richedit_original_hwnd_, EM_SETBKGNDCOLOR, 0, RGB(245, 245, 245));
	SendMessageA(richedit_original_hwnd_, EM_SETLIMITTEXT, 0x100000, 0);
	SendMessageA(richedit_original_hwnd_, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
	SendMessageA(richedit_original_hwnd_, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
}

void editor_app_t::position_richedit(float screen_x, float screen_y, float width, float height)
{
	if (!richedit_hwnd_)
		return;

	float adjusted_width = width;

	bool should_show = (adjusted_width > 10.0f && height > 20.0f);

	if (should_show)
	{
		SetWindowPos(
		    richedit_hwnd_,
		    HWND_TOP,
		    static_cast<int>(screen_x),
		    static_cast<int>(screen_y),
		    static_cast<int>(adjusted_width),
		    static_cast<int>(height),
		    SWP_NOACTIVATE | (richedit_visible_ ? 0 : SWP_SHOWWINDOW));
		if (!richedit_visible_)
			ShowWindow(richedit_hwnd_, SW_SHOWNOACTIVATE);
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

	HWND prev_focus = GetFocus();
	int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
	std::wstring wtext(wlen, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), wtext.data(), wlen);
	SetWindowTextW(richedit_hwnd_, wtext.c_str());
	if (prev_focus && prev_focus != richedit_hwnd_)
		SetFocus(prev_focus);
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
		prev_entry.status = tools_t::status_t::error;
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

void editor_app_t::highlight_richedit_syntax(const std::string & text, tools_t::rec_type_t type)
{
	if (!richedit_hwnd_)
		return;

	auto tokens = syntax_.tokenize(text, type);
	if (tokens.empty())
		return;

	std::vector<int> byte_to_wchar(text.size() + 1, 0);
	int wchar_pos = 0;
	for (size_t i = 0; i < text.size();)
	{
		byte_to_wchar[i] = wchar_pos;
		unsigned char c = static_cast<unsigned char>(text[i]);

		if (c == '\n' && i > 0 && text[i - 1] == '\r')
		{
			i++;
			continue;
		}

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

		for (int j = 1; j < char_len && i + j < text.size(); ++j)
		{
			byte_to_wchar[i + j] = wchar_pos;
			codepoint = (codepoint << 6) | (static_cast<unsigned char>(text[i + j]) & 0x3F);
		}

		wchar_pos += (codepoint > 0xFFFF) ? 2 : 1;
		i += char_len;
	}
	byte_to_wchar[text.size()] = wchar_pos;

	SendMessageA(richedit_hwnd_, WM_SETREDRAW, FALSE, 0);

	CHARRANGE orig_sel;
	SendMessageA(richedit_hwnd_, EM_EXGETSEL, 0, (LPARAM)&orig_sel);

	for (const auto & token : tokens)
	{
		if (token.type == token_type_t::normal)
			continue;

		if (token.start >= token.end || token.start >= text.size())
			continue;

		size_t end = std::min(token.end, text.size());

		COLORREF color = RGB(255, 255, 255);
		switch (token.type)
		{
		case token_type_t::mwscript_function:
			color = RGB(100, 180, 255);
			break;
		case token_type_t::mwscript_comment:
			color = RGB(128, 128, 128);
			break;
		case token_type_t::mwscript_string:
			color = RGB(200, 150, 50);
			break;
		case token_type_t::html_tag:
			color = RGB(140, 40, 50);
			break;
		default:
			continue;
		}

		CHARRANGE range;
		range.cpMin = byte_to_wchar[token.start];
		range.cpMax = byte_to_wchar[end];
		SendMessageA(richedit_hwnd_, EM_EXSETSEL, 0, (LPARAM)&range);

		CHARFORMAT2A cf = {};
		cf.cbSize = sizeof(cf);
		cf.dwMask = CFM_COLOR;
		cf.dwEffects = 0;
		cf.crTextColor = color;
		SendMessageA(richedit_hwnd_, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
	}

	SendMessageA(richedit_hwnd_, EM_EXSETSEL, 0, (LPARAM)&orig_sel);
	SendMessageA(richedit_hwnd_, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(richedit_hwnd_, nullptr, TRUE);
}

void editor_app_t::highlight_richedit_spelling(const std::string & text)
{
	if (!richedit_hwnd_ || !spell_checker_.is_loaded())
		return;

	auto misspelled = spell_checker_.find_misspelled(text);
	if (misspelled.empty())
		return;

	std::vector<int> byte_to_wchar(text.size() + 1, 0);
	int wchar_pos = 0;
	for (size_t i = 0; i < text.size();)
	{
		byte_to_wchar[i] = wchar_pos;
		unsigned char c = static_cast<unsigned char>(text[i]);

		if (c == '\n' && i > 0 && text[i - 1] == '\r')
		{
			i++;
			continue;
		}

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

		for (int j = 1; j < char_len && i + j < text.size(); ++j)
		{
			byte_to_wchar[i + j] = wchar_pos;
			codepoint = (codepoint << 6) | (static_cast<unsigned char>(text[i + j]) & 0x3F);
		}

		wchar_pos += (codepoint > 0xFFFF) ? 2 : 1;
		i += char_len;
	}
	byte_to_wchar[text.size()] = wchar_pos;

	SendMessageA(richedit_hwnd_, WM_SETREDRAW, FALSE, 0);

	CHARRANGE orig_sel;
	SendMessageA(richedit_hwnd_, EM_EXGETSEL, 0, (LPARAM)&orig_sel);

	for (const auto & match : misspelled)
	{
		if (match.start >= text.size() || match.end > text.size())
			continue;

		CHARRANGE range;
		range.cpMin = byte_to_wchar[match.start];
		range.cpMax = byte_to_wchar[match.end];
		SendMessageA(richedit_hwnd_, EM_EXSETSEL, 0, (LPARAM)&range);

		CHARFORMAT2A cf = {};
		cf.cbSize = sizeof(cf);
		cf.dwMask = CFM_UNDERLINETYPE | CFM_UNDERLINE;
		cf.dwEffects = CFE_UNDERLINE;
		cf.bUnderlineType = CFU_UNDERLINEWAVE;
		cf.bUnderlineColor = 0x06;
		SendMessageA(richedit_hwnd_, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
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
