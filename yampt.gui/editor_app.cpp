#include "editor_app.hpp"
#include "imgui.h"

#include <algorithm>

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
}

void editor_app_t::frame()
{
    rebuild_row_data();

    render_menu_bar();
    render_toolbar();
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
        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
            left_rows_.push_back({type, i});
            left_lookup_[make_lookup_key(type, chapter.records[i].key_text)] = idx;
            ++idx;
        }
    }

    idx = 0;
    for (const auto & [type, chapter] : state_.get_source_dict())
    {
        for (size_t i = 0; i < chapter.records.size(); ++i)
        {
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
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void editor_app_t::render_toolbar()
{
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

    if (!ImGui::BeginTable(table_id, 4, flags))
        return;

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, config_.column_widths[0]);
    ImGui::TableSetupColumn("Original", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Translation", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, config_.column_widths[3]);
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

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(entry.old_text.c_str());

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
                        state_.mark_modified(row_ref.type, row_ref.record_index);
                    }
                    editing_row_ = -1;
                }

                if (cancel)
                    editing_row_ = -1;

                ImGui::PopItemWidth();
            }
            else
            {
                ImGui::TextUnformatted(entry.new_text.c_str());

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
            ImGui::TextUnformatted(entry.status.c_str());

            ImGui::PopID();
        }
    }

    if (is_left_panel)
    {
    }

    ImGui::EndTable();
}

void editor_app_t::render_status_bar()
{
}

void editor_app_t::render_annotations_panel()
{
}

void editor_app_t::render_history_panel()
{
}

void editor_app_t::render_dialogs()
{
    if (show_quit_dialog_)
        render_quit_dialog();
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
