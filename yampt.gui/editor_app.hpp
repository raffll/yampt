#pragma once

#include "editor_config.hpp"
#include "editor_state.hpp"
#include <SDL.h>
#include <string>
#include <unordered_map>
#include <vector>

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

    int selected_row_left_ = -1;
    int selected_row_right_ = -1;
    int scroll_to_row_left_ = -1;
    int scroll_to_row_right_ = -1;

    int editing_row_ = -1;
    std::vector<char> edit_buffer_;
    bool edit_multiline_ = false;

    struct row_ref_t
    {
        tools_t::rec_type_t type;
        size_t record_index;
    };

    std::vector<row_ref_t> left_rows_;
    std::vector<row_ref_t> right_rows_;
    std::unordered_map<std::string, int> left_lookup_;
    std::unordered_map<std::string, int> right_lookup_;

    void rebuild_row_data();
    static std::string make_lookup_key(tools_t::rec_type_t type, const std::string & key);

    void render_menu_bar();
    void render_toolbar();
    void render_panels();
    void render_status_bar();
    void render_annotations_panel();
    void render_history_panel();
    void render_dialogs();
    void render_quit_dialog();

    std::string get_exe_directory() const;

    void render_dict_table(
        const char * table_id,
        const tools_t::dict_t & dict,
        tools_t::dict_t * mutable_dict,
        bool is_left_panel);

    std::string show_open_file_dialog(const char * title) const;
    std::string show_save_file_dialog(const char * title) const;
};
