#pragma once

#include "annotation_manager.hpp"
#include "base_dict_manager.hpp"
#include "editor_config.hpp"
#include "editor_state.hpp"
#include "history_manager.hpp"
#include "search_manager.hpp"
#include "syntax_highlighter.hpp"
#include "validation_manager.hpp"
#include <SDL.h>
#include <array>
#include <set>
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
    validation_manager_t validation_;

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

    bool show_goto_dialog_ = false;
    int goto_row_value_ = 0;

    bool show_base_dict_config_ = false;
    bool show_auto_translate_result_ = false;
    auto_translate_result_t last_auto_result_;
    std::string last_auto_result_title_;

    std::set<tools_t::rec_type_t> type_filter_ = {
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

    std::set<std::string> status_filter_;

    search_manager_t search_;
    history_manager_t history_;
    syntax_highlighter_t syntax_;
    annotation_manager_t annotations_mgr_;
    base_dict_manager_t base_dicts_;
    std::array<char, 256> search_buffer_ = {};
    bool search_case_sensitive_ = false;

    std::array<char, 256> replace_buffer_ = {};
    bool show_replace_ = false;
    size_t last_replace_count_ = 0;

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
    void render_status_summary_bar();
    void render_panels();
    void render_status_bar();
    void render_annotations_panel();
    void render_history_panel();
    void render_dialogs();
    void render_quit_dialog();
    void render_goto_dialog();
    void render_base_dict_config();
    void render_auto_translate_result_dialog();

    std::string get_exe_directory() const;

    void render_dict_table(
        const char * table_id,
        const tools_t::dict_t & dict,
        tools_t::dict_t * mutable_dict,
        bool is_left_panel);

    void render_text_with_highlights(const std::string & text, tools_t::rec_type_t type, size_t record_index, bool is_key);
    void render_text_with_syntax(const std::string & text, tools_t::rec_type_t type);
    void render_text_with_topic_highlights(const std::string & text);

    std::string show_open_file_dialog(const char * title) const;
    std::string show_save_file_dialog(const char * title) const;
};
