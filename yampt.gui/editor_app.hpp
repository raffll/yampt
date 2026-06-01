#pragma once

#include "editor_config.hpp"
#include "editor_state.hpp"
#include <SDL.h>

class editor_app_t
{
public:
    void init(SDL_Window * window);
    void frame();
    void shutdown();
    bool wants_quit() const;
    void request_quit();

private:
    SDL_Window * window_ = nullptr;
    editor_config_t config_;
    std::string config_path_;
    editor_state_t state_;

    float split_ratio_ = 0.5f;
    bool show_history_ = false;
    bool show_annotations_ = true;

    bool quit_requested_ = false;
    bool show_quit_dialog_ = false;

    void render_menu_bar();
    void render_toolbar();
    void render_panels();
    void render_status_bar();
    void render_annotations_panel();
    void render_history_panel();
    void render_dialogs();
    void render_quit_dialog();

    std::string get_exe_directory() const;

    std::string show_open_file_dialog(const char * title) const;
    std::string show_save_file_dialog(const char * title) const;
};
