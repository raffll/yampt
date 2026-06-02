#define NOMINMAX

#include "editor_app.hpp"
#include "imgui.h"
#include "status_colors.hpp"

#include <algorithm>
#include <array>

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

    if (!config_.base_dict_paths.empty())
    {
        base_dicts_.set_paths(config_.base_dict_paths);
        base_dicts_.reload();
    }
}

void editor_app_t::frame()
{
    std::string title = "yampt.gui";
    if (state_.has_unsaved_changes())
        title += " *";
    SDL_SetWindowTitle(window_, title.c_str());

    rebuild_row_data();

    render_menu_bar();
    render_toolbar();
    render_status_summary_bar();
    render_panels();
    render_status_bar();

    if (show_annotations_)
        render_annotations_panel();

    if (show_history_)
        render_history_panel();

    render_dialogs();
}

void editor_app_t::shutdown()
{
    config_.split_ratio = split_ratio_;
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

    int idx = 0;
    for (const auto & [type, chapter] : state_.get_user_dict())
    {
        if (type_filter_.count(type) == 0)
            continue;

        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            if (!status_filter_.empty() && status_filter_.count(chapter.records[i].status) == 0)
                continue;

            left_rows_.push_back({type, i});
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
            if (!status_filter_.empty() && status_filter_.count(chapter.records[i].status) == 0)
                continue;

            right_rows_.push_back({type, i});
            right_lookup_[make_lookup_key(type, chapter.records[i].key_text)] = idx;
            ++idx;
        }
    }
}

void editor_app_t::render_menu_bar()
{
    if (!ImGui::BeginMainMenuBar())
        return;

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Open User Dict...", "Ctrl+O"))
        {
            auto path = show_open_file_dialog("Open User Dictionary");
            if (!path.empty())
            {
                state_.load_user_dict(path);
                annotations_mgr_.rebuild(state_);
                selected_row_left_ = -1;
                selected_row_right_ = -1;
                editing_row_ = -1;
            }
        }

        if (ImGui::MenuItem("Open Source Dict..."))
        {
            auto path = show_open_file_dialog("Open Source Dictionary");
            if (!path.empty())
            {
                state_.load_source_dict(path);
                selected_row_left_ = -1;
                selected_row_right_ = -1;
            }
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Save", "Ctrl+S"))
        {
            if (state_.get_user_path().empty())
            {
                auto path = show_save_file_dialog("Save User Dictionary");
                if (!path.empty())
                    state_.save_user_dict_as(path);
            }
            else
            {
                state_.save_user_dict();
            }
        }

        if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
        {
            auto path = show_save_file_dialog("Save User Dictionary As");
            if (!path.empty())
                state_.save_user_dict_as(path);
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Load Glossary..."))
        {
            auto path = show_open_file_dialog("Load Glossary Dictionary");
            if (!path.empty())
                annotations_mgr_.load_glossary(path);
        }

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
        ImGui::MenuItem("Annotations", nullptr, &show_annotations_);
        ImGui::MenuItem("History", nullptr, &show_history_);
        ImGui::Separator();
        ImGui::MenuItem("Base Dictionary Config", nullptr, &show_base_dict_config_);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Tools"))
    {
        if (ImGui::MenuItem("Auto-translate from Base"))
        {
            auto r = base_dicts_.auto_translate_from_base(state_);
            last_auto_result_ = r;
            last_auto_result_title_ = "Auto-translate from Base";
            show_auto_translate_result_ = true;
            rebuild_row_data();
        }

        if (ImGui::MenuItem("Auto-translate Identical"))
        {
            auto r = base_dicts_.auto_translate_identical(state_);
            last_auto_result_ = r;
            last_auto_result_title_ = "Auto-translate Identical";
            show_auto_translate_result_ = true;
            rebuild_row_data();
        }

        if (ImGui::MenuItem("Auto-translate Heuristic"))
        {
            auto r = base_dicts_.auto_translate_heuristic(state_);
            last_auto_result_ = r;
            last_auto_result_title_ = "Auto-translate Heuristic";
            show_auto_translate_result_ = true;
            rebuild_row_data();
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Detect Changed in Base"))
        {
            base_dicts_.detect_changed(state_);
            rebuild_row_data();
        }

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void editor_app_t::render_toolbar()
{
    static constexpr tools_t::rec_type_t filter_types[] = {
        tools_t::rec_type_t::CELL,
        tools_t::rec_type_t::DIAL,
        tools_t::rec_type_t::INFO,
        tools_t::rec_type_t::FNAM,
        tools_t::rec_type_t::TEXT,
        tools_t::rec_type_t::GMST,
        tools_t::rec_type_t::DESC,
        tools_t::rec_type_t::RNAM,
        tools_t::rec_type_t::INDX,
        tools_t::rec_type_t::BNAM,
        tools_t::rec_type_t::SCTX,
    };

    float toolbar_height = show_replace_ ? 80.0f : 55.0f;
    ImGui::BeginChild("Toolbar", ImVec2(0, toolbar_height), ImGuiChildFlags_None);

    for (size_t i = 0; i < std::size(filter_types); ++i)
    {
        if (i > 0)
            ImGui::SameLine();

        auto type = filter_types[i];
        bool active = type_filter_.count(type) > 0;

        size_t count = 0;
        auto & dict = state_.get_user_dict();
        auto it = dict.find(type);
        if (it != dict.end())
            count = it->second.records.size();

        std::string label = tools_t::type2Str(type) + " (" + std::to_string(count) + ")";

        if (ImGui::Checkbox(label.c_str(), &active))
        {
            if (active)
                type_filter_.insert(type);
            else
                type_filter_.erase(type);
        }
    }

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
        std::string match_info = std::to_string(search_.current_index() + 1)
                               + "/" + std::to_string(search_.get_matches().size());
        ImGui::TextUnformatted(match_info.c_str());
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
                    break;
                }
            }
        }
    }

    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_H))
    {
        show_replace_ = !show_replace_;
    }

    if (show_replace_)
    {
        ImGui::SetNextItemWidth(200.0f);
        bool replace_enter = ImGui::InputText("Replace", replace_buffer_.data(), replace_buffer_.size(), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (ImGui::Button("Replace") || replace_enter)
        {
            search_.replace_current(state_, std::string(replace_buffer_.data()));
            search_.find_all(state_, type_filter_);
            search_.next_match();
        }
        ImGui::SameLine();
        if (ImGui::Button("Replace All"))
        {
            last_replace_count_ = search_.replace_all(state_, std::string(replace_buffer_.data()));
            search_.find_all(state_, type_filter_);
            annotations_mgr_.rebuild(state_);
            rebuild_row_data();
        }
    }

    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_G))
    {
        show_goto_dialog_ = true;
        goto_row_value_ = selected_row_left_ >= 0 ? selected_row_left_ + 1 : 1;
        ImGui::OpenPopup("Go to Row");
    }

    ImGui::EndChild();
}

void editor_app_t::render_status_summary_bar()
{
    static const char * status_names[] = { "", "translated", "auto_identical", "auto_heuristic", "validated", "changed", "has_errors" };
    static const char * status_labels[] = { "Untranslated", "Translated", "Auto Identical", "Auto Heuristic", "Validated", "Changed", "Has Errors" };
    static constexpr size_t status_count = 7;

    int counts[status_count] = {};

    for (const auto & [type, chapter] : state_.get_user_dict())
    {
        if (type_filter_.count(type) == 0)
            continue;

        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            const auto & status = chapter.records[i].status;

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

        ImVec4 color = get_status_color(status_names[i]);
        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));

        std::string label = std::string(status_labels[i]) + ": " + std::to_string(counts[i]);
        ImGui::SmallButton(label.c_str());

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            std::string status = status_names[i];
            if (status_filter_.size() == 1 && status_filter_.count(status) > 0)
                status_filter_.clear();
            else
            {
                status_filter_.clear();
                status_filter_.insert(status);
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
            else
            {
                status_filter_.erase(status);
                if (status_filter_.empty())
                    status_filter_.clear();
            }
            rebuild_row_data();
        }

        ImGui::PopStyleColor(4);
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

    const ImGuiTableFlags flags = ImGuiTableFlags_Resizable
                                | ImGuiTableFlags_ScrollY
                                | ImGuiTableFlags_RowBg
                                | ImGuiTableFlags_BordersOuter
                                | ImGuiTableFlags_BordersV;

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
            std::string id_text = tools_t::type2Str(row_ref.type) + ": " + entry.key_text;
            ImGui::PushID(i);

            if (ImGui::Selectable(
                    id_text.c_str(), is_selected,
                    ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick,
                    ImVec2(0, 0)))
            {
                selected_row = i;

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
                    }
                }
            }

            bool double_clicked = ImGui::IsItemHovered()
                               && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

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
                            rec.status = "";
                            state_.mark_modified(row_ref.type, row_ref.record_index);
                        }
                        if (ImGui::MenuItem("Set Translated"))
                        {
                            rec.status = tools_t::Status::translated;
                            state_.mark_modified(row_ref.type, row_ref.record_index);
                        }
                        if (ImGui::MenuItem("Set Validated"))
                        {
                            rec.status = "validated";
                            state_.mark_modified(row_ref.type, row_ref.record_index);
                        }
                        if (ImGui::MenuItem("Set Changed"))
                        {
                            rec.status = tools_t::Status::changed;
                            state_.mark_modified(row_ref.type, row_ref.record_index);
                        }

                        ImGui::EndPopup();
                    }
                }
            }

            ImGui::TableSetColumnIndex(1);
            if (search_.get_matches().empty())
            {
                if (row_ref.type == tools_t::rec_type_t::SCTX || row_ref.type == tools_t::rec_type_t::TEXT)
                    render_text_with_syntax(entry.old_text, row_ref.type);
                else if (row_ref.type == tools_t::rec_type_t::INFO)
                    render_text_with_topic_highlights(entry.old_text);
                else
                    ImGui::TextUnformatted(entry.old_text.c_str());
            }
            else
                render_text_with_highlights(entry.old_text, row_ref.type, row_ref.record_index, true);

            ImGui::TableSetColumnIndex(2);

            bool is_editing = is_left_panel && mutable_dict && (editing_row_ == i);

            if (is_editing)
            {
                ImGui::PushItemWidth(-1);
                bool commit = false;
                bool cancel = false;

                if (edit_multiline_)
                {
                    ImGui::InputTextMultiline(
                        "##edit", edit_buffer_.data(), edit_buffer_.size(),
                        ImVec2(-1, row_height * 4),
                        ImGuiInputTextFlags_AllowTabInput);

                    if (ImGui::IsKeyPressed(ImGuiKey_Enter)
                        && ImGui::GetIO().KeyCtrl)
                        commit = true;
                    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                        cancel = true;
                }
                else
                {
                    if (ImGui::InputText(
                            "##edit", edit_buffer_.data(), edit_buffer_.size(),
                            ImGuiInputTextFlags_EnterReturnsTrue))
                        commit = true;
                    if (ImGui::IsKeyPressed(ImGuiKey_Escape))
                        cancel = true;
                }

                if (commit)
                {
                    std::string new_value(edit_buffer_.data());
                    auto mit = mutable_dict->find(row_ref.type);
                    if (mit != mutable_dict->end()
                        && row_ref.record_index < mit->second.records.size())
                    {
                        auto & rec = mit->second.records[row_ref.record_index];
                        rec.new_text = new_value;
                        rec.status = tools_t::Status::translated;
                        auto vr = validation_.validate(row_ref.type, new_value);
                        if (vr.level == validation_level_t::error)
                            rec.status = tools_t::Status::has_errors;
                        state_.mark_modified(row_ref.type, row_ref.record_index);
                        if (row_ref.type == tools_t::rec_type_t::DIAL)
                            annotations_mgr_.rebuild(state_);
                    }
                    editing_row_ = -1;
                }

                if (cancel)
                    editing_row_ = -1;

                ImGui::PopItemWidth();
            }
            else
            {
                if (search_.get_matches().empty())
                {
                    if (row_ref.type == tools_t::rec_type_t::SCTX || row_ref.type == tools_t::rec_type_t::TEXT)
                        render_text_with_syntax(entry.new_text, row_ref.type);
                    else if (row_ref.type == tools_t::rec_type_t::INFO)
                        render_text_with_topic_highlights(entry.new_text);
                    else
                        ImGui::TextUnformatted(entry.new_text.c_str());
                }
                else
                    render_text_with_highlights(entry.new_text, row_ref.type, row_ref.record_index, false);

                if (double_clicked && is_left_panel && mutable_dict)
                {
                    editing_row_ = i;
                    edit_multiline_ = (row_ref.type == tools_t::rec_type_t::TEXT
                                    || row_ref.type == tools_t::rec_type_t::INFO);
                    edit_buffer_.resize(EDIT_BUFFER_SIZE);
                    std::fill(edit_buffer_.begin(), edit_buffer_.end(), '\0');
                    size_t copy_len = entry.new_text.size();
                    if (copy_len > EDIT_BUFFER_SIZE - 1)
                        copy_len = EDIT_BUFFER_SIZE - 1;
                    std::copy_n(entry.new_text.begin(), copy_len, edit_buffer_.begin());
                }
            }

            ImGui::TableSetColumnIndex(3);
            ImVec4 status_color = get_status_color(entry.status);
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::ColorConvertFloat4ToU32(status_color));
            ImGui::TextUnformatted(entry.status.c_str());

            ImGui::TableSetColumnIndex(4);
            if (row_ref.type == tools_t::rec_type_t::FNAM && annotations_mgr_.has_enchantment(entry.key_text))
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
                        std::string tip = std::to_string(result.byte_count) + " / " + std::to_string(result.limit) + " bytes";
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
                        std::string tip = std::to_string(result.byte_count) + " / " + std::to_string(result.limit) + " bytes (OVER LIMIT)";
                        ImGui::SetTooltip("%s", tip.c_str());
                    }
                }
            }

            ImGui::PopID();
        }
    }

    if (is_left_panel)
    {
    }

    ImGui::EndTable();
}

void editor_app_t::render_text_with_highlights(const std::string & text, tools_t::rec_type_t type, size_t record_index, bool is_key)
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

    std::sort(relevant.begin(), relevant.end(), [](const search_match_t * a, const search_match_t * b)
    {
        return a->char_start < b->char_start;
    });

    ImDrawList * draw_list = ImGui::GetWindowDrawList();
    ImVec4 highlight_color(1.0f, 0.9f, 0.2f, 0.4f);
    ImU32 highlight_u32 = ImGui::ColorConvertFloat4ToU32(highlight_color);

    size_t pos = 0;
    bool first_segment = true;

    for (const auto * m : relevant)
    {
        size_t start = m->char_start;
        size_t end = m->char_end;

        if (start > text.size())
            start = text.size();
        if (end > text.size())
            end = text.size();
        if (start < pos)
            start = pos;
        if (end <= start)
            continue;

        if (start > pos)
        {
            if (!first_segment)
                ImGui::SameLine(0, 0);
            ImGui::TextUnformatted(text.c_str() + pos, text.c_str() + start);
            first_segment = false;
        }

        if (!first_segment)
            ImGui::SameLine(0, 0);

        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImVec2 text_size = ImGui::CalcTextSize(text.c_str() + start, text.c_str() + end);
        draw_list->AddRectFilled(cursor, ImVec2(cursor.x + text_size.x, cursor.y + text_size.y), highlight_u32);
        ImGui::TextUnformatted(text.c_str() + start, text.c_str() + end);
        first_segment = false;

        pos = end;
    }

    if (pos < text.size())
    {
        if (!first_segment)
            ImGui::SameLine(0, 0);
        ImGui::TextUnformatted(text.c_str() + pos, text.c_str() + text.size());
    }
}

void editor_app_t::render_text_with_syntax(const std::string & text, tools_t::rec_type_t type)
{
    auto tokens = syntax_.tokenize(text, type);

    if (tokens.empty())
    {
        ImGui::TextUnformatted(text.c_str());
        return;
    }

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
}

void editor_app_t::render_text_with_topic_highlights(const std::string & text)
{
    auto annotations = annotations_mgr_.annotate(text, tools_t::rec_type_t::INFO);

    if (annotations.empty())
    {
        ImGui::TextUnformatted(text.c_str());
        return;
    }

    std::sort(annotations.begin(), annotations.end(),
              [](const annotation_t & a, const annotation_t & b)
              {
                  return a.start < b.start;
              });

    ImVec4 topic_color(0.3f, 0.5f, 0.8f, 1.0f);
    size_t pos = 0;
    bool first_segment = true;

    for (const auto & ann : annotations)
    {
        if (ann.start < pos)
            continue;

        if (ann.start > pos)
        {
            if (!first_segment)
                ImGui::SameLine(0, 0);
            ImGui::TextUnformatted(text.c_str() + pos, text.c_str() + ann.start);
            first_segment = false;
        }

        if (!first_segment)
            ImGui::SameLine(0, 0);

        ImGui::PushStyleColor(ImGuiCol_Text, topic_color);
        ImGui::TextUnformatted(text.c_str() + ann.start, text.c_str() + ann.end);
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s", ann.translated.c_str());

        first_segment = false;
        pos = ann.end;
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

    if (last_replace_count_ > 0)
    {
        std::string msg = "Replaced " + std::to_string(last_replace_count_) + " occurrence(s)";
        ImGui::TextUnformatted(msg.c_str());
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
    }

    std::string info = "Records: " + std::to_string(left_rows_.size());
    ImGui::TextUnformatted(info.c_str());

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

    const auto & row = left_rows_[selected_row_left_];
    auto it = state_.get_user_dict().find(row.type);
    if (it == state_.get_user_dict().end() || row.record_index >= it->second.records.size())
    {
        ImGui::EndChild();
        return;
    }

    const auto & entry = it->second.records[row.record_index];

    if (row.type == tools_t::rec_type_t::FNAM && annotations_mgr_.has_enchantment(entry.key_text))
    {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "\xe2\x9a\xa1 Enchantment");
        const auto & ench_name = annotations_mgr_.get_enchantment(entry.key_text);
        ImGui::SameLine();
        ImGui::TextUnformatted(ench_name.c_str());
        ImGui::Separator();
    }

    const auto & base_dict = base_dicts_.get_merged_dict();
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

    if (entry.status == tools_t::Status::changed && base_it != base_dict.end())
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

    auto fuzzy = base_dicts_.find_fuzzy_matches(entry.old_text, row.type);
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

    if (row.type != tools_t::rec_type_t::INFO)
    {
        ImGui::EndChild();
        return;
    }

    auto annotations = annotations_mgr_.annotate(entry.new_text, row.type);

    std::vector<const annotation_t *> hyperlinks;
    std::vector<const annotation_t *> glossary;

    for (const auto & ann : annotations)
    {
        if (ann.kind == annotation_t::dial_topic)
            hyperlinks.push_back(&ann);
        else if (ann.kind == annotation_t::glossary_term)
            glossary.push_back(&ann);
    }

    if (!hyperlinks.empty())
    {
        ImGui::TextColored(ImVec4(0.3f, 0.5f, 0.8f, 1.0f), "Hyperlinks");
        for (const auto * ann : hyperlinks)
        {
            std::string label = ann->original + " -> " + ann->translated;
            if (ImGui::Selectable(label.c_str()))
                ImGui::SetClipboardText(ann->translated.c_str());
        }
        ImGui::Separator();
    }

    if (!glossary.empty())
    {
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 0.4f, 1.0f), "Glossary");
        for (const auto * ann : glossary)
        {
            std::string label = ann->original + " -> " + ann->translated;
            if (ImGui::Selectable(label.c_str()))
                ImGui::SetClipboardText(ann->translated.c_str());
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

    const auto & row = left_rows_[selected_row_left_];
    auto it = state_.get_user_dict().find(row.type);
    if (it == state_.get_user_dict().end() || row.record_index >= it->second.records.size())
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
                history_.revert(state_, row.type, entry.key_text, i);
                rebuild_row_data();
            }
        }

        ImGui::SameLine();
        if (ImGui::SmallButton("Revert"))
        {
            history_.revert(state_, row.type, entry.key_text, i);
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

    if (show_goto_dialog_)
        render_goto_dialog();

    if (show_base_dict_config_)
        render_base_dict_config();

    if (show_auto_translate_result_)
        render_auto_translate_result_dialog();
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
                state_.save_user_dict_as(path);
        }
        else
        {
            state_.save_user_dict();
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

void editor_app_t::render_goto_dialog()
{
    ImGui::OpenPopup("Go to Row");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (!ImGui::BeginPopupModal("Go to Row", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    ImGui::InputInt("Row", &goto_row_value_);

    if (ImGui::Button("OK", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter))
    {
        int max_row = static_cast<int>(left_rows_.size());
        int target = std::clamp(goto_row_value_, 1, std::max(max_row, 1)) - 1;
        scroll_to_row_left_ = target;
        selected_row_left_ = target;
        show_goto_dialog_ = false;
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(120, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
    {
        show_goto_dialog_ = false;
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
        base_dicts_.set_paths(config_.base_dict_paths);
        base_dicts_.reload();
        config_.save(config_path_);
    }

    ImGui::End();
}

void editor_app_t::render_auto_translate_result_dialog()
{
    ImGui::OpenPopup("Auto-translate Result");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (!ImGui::BeginPopupModal("Auto-translate Result", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    ImGui::Text("%s", last_auto_result_title_.c_str());
    ImGui::Separator();

    std::string translated_str = "Translated: " + std::to_string(last_auto_result_.translated);
    std::string no_match_str = "Skipped (no match): " + std::to_string(last_auto_result_.skipped_no_match);
    std::string changed_str = "Skipped (text changed): " + std::to_string(last_auto_result_.skipped_text_changed);

    ImGui::TextUnformatted(translated_str.c_str());
    ImGui::TextUnformatted(no_match_str.c_str());
    ImGui::TextUnformatted(changed_str.c_str());

    ImGui::Separator();

    if (ImGui::Button("OK", ImVec2(120, 0)))
    {
        show_auto_translate_result_ = false;
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
