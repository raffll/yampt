#include "editor_app.hpp"
#include "imgui.h"

#include <algorithm>

#include <Windows.h>
#include <commdlg.h>

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
                state_.load_user_dict(path);
        }

        if (ImGui::MenuItem("Open Source Dict..."))
        {
            auto path = show_open_file_dialog("Open Source Dictionary");
            if (!path.empty())
                state_.load_source_dict(path);
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

    ImGui::BeginChild("LeftPanel", ImVec2(left_width, available.y), true);
    ImGui::Text("Left Panel");
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::Button("##splitter", ImVec2(splitter_width, available.y));
    if (ImGui::IsItemActive())
        split_ratio_ += ImGui::GetIO().MouseDelta.x / available.x;
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    split_ratio_ = std::clamp(split_ratio_, 0.1f, 0.9f);

    ImGui::SameLine();

    ImGui::BeginChild("RightPanel", ImVec2(right_width, available.y), true);
    ImGui::Text("Right Panel");
    ImGui::EndChild();
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
