#pragma once

#include "annotation_manager.hpp"
#include "dict_workspace.hpp"
#include "editor_config.hpp"
#include "encoding_utils.hpp"
#include "history_manager.hpp"
#include "search_manager.hpp"
#include "spell_checker.hpp"
#include "syntax_highlighter.hpp"
#include "validation_manager.hpp"
#include <SDL.h>
#include <array>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <Windows.h>

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
	dict_workspace_t workspace_;
	validation_manager_t validation_;

	float split_ratio_ = 0.5f;
	float sidebar_width_ = 250.0f;
	float bottom_height_ = 200.0f;
	float info_height_ = 150.0f;
	bool sidebar_visible_ = true;
	bool bottom_visible_ = true;
	int active_bottom_tab_ = 0;
	int encoding_index_ = 2;

	bool quit_requested_ = false;
	bool show_quit_dialog_ = false;
	bool show_unload_confirm_ = false;
	int pending_unload_index_ = -1;
	bool show_whitespace_ = false;

	int selected_row_ = -1;
	int scroll_to_row_ = -1;

	int selected_row_left_ = -1;
	int scroll_to_row_left_ = -1;

	int editing_row_ = -1;
	tools_t::rec_type_t editing_type_ = tools_t::rec_type_t::unknown;
	size_t editing_record_index_ = 0;
	std::vector<char> edit_buffer_;
	bool edit_multiline_ = false;
	bool edit_focus_pending_ = false;

	int spell_ctx_row_ = -1;
	std::string spell_ctx_word_;
	size_t spell_ctx_start_ = 0;
	size_t spell_ctx_end_ = 0;

	std::set<tools_t::rec_type_t> type_filter_ = {
		tools_t::rec_type_t::cell, tools_t::rec_type_t::dial, tools_t::rec_type_t::info, tools_t::rec_type_t::fnam,
		tools_t::rec_type_t::text, tools_t::rec_type_t::gmst, tools_t::rec_type_t::desc, tools_t::rec_type_t::rnam,
		tools_t::rec_type_t::indx, tools_t::rec_type_t::bnam, tools_t::rec_type_t::sctx,
	};

	static constexpr tools_t::rec_type_t all_types_[] = {
		tools_t::rec_type_t::cell, tools_t::rec_type_t::dial, tools_t::rec_type_t::info, tools_t::rec_type_t::fnam,
		tools_t::rec_type_t::text, tools_t::rec_type_t::gmst, tools_t::rec_type_t::desc, tools_t::rec_type_t::rnam,
		tools_t::rec_type_t::indx, tools_t::rec_type_t::bnam, tools_t::rec_type_t::sctx,
	};

	tools_t::rec_type_t sidebar_active_type_ = tools_t::rec_type_t::unknown;

	std::set<std::string> status_filter_;
	std::set<std::string> saved_status_filter_;
	bool status_filter_solo_ = false;

	std::set<tools_t::rec_type_t> saved_type_filter_;
	bool type_filter_solo_ = false;

	search_manager_t search_;
	history_manager_t history_;
	syntax_highlighter_t syntax_;
	annotation_manager_t annotations_mgr_;
	spell_checker_t spell_checker_;
	HWND richedit_hwnd_ = nullptr;
	bool richedit_visible_ = false;
	bool richedit_ignore_change_ = false;
	std::array<char, 256> search_buffer_ = {};
	bool search_case_sensitive_ = false;

	std::array<char, 256> speaker_filter_buffer_ = {};

	struct row_ref_t
	{
		tools_t::rec_type_t type;
		size_t record_index;
	};

	std::vector<row_ref_t> left_rows_;
	std::unordered_map<std::string, int> left_lookup_;

	void rebuild_row_data();
	void rebuild_annotations();
	static std::string make_lookup_key(tools_t::rec_type_t type, const std::string & key);

	void render_menu_bar();
	void render_toolbar();
	void render_sidebar();
	void render_status_summary_bar();
	void render_main_panel();
	void render_status_bar();
	void render_bottom_panel();
	void render_info_panel();
	void render_editor_tab();
	void render_annotations_tab();
	void render_history_tab();
	void render_speaker_tab();
	void render_annotations_panel();
	void render_history_panel();
	void render_dialogs();
	void render_quit_dialog();
	void render_unload_confirm_dialog();

	std::string get_exe_directory() const;

	void render_text_with_highlights(
	    const std::string & text,
	    tools_t::rec_type_t type,
	    size_t record_index,
	    bool is_key);
	void render_text_with_syntax(const std::string & text, tools_t::rec_type_t type);
	void render_text_with_topic_highlights(const std::string & text);
	void render_translation_with_spellcheck(const std::string & text, int row_index);

	std::string show_open_file_dialog(const char * title) const;
	std::string show_save_file_dialog(const char * title) const;

	void create_richedit(HWND parent);
	void position_richedit(float screen_x, float screen_y, float width, float height);
	void set_richedit_text(const std::string & text);
	std::string get_richedit_text() const;
	void commit_richedit_text();
	void highlight_richedit_hyperlinks(
	    const std::string & text,
	    tools_t::rec_type_t type,
	    const std::string & original_text);

	void render_splitter_vertical(float & width, float min_w, float max_w);
	void render_splitter_horizontal(float & height, float min_h, float max_h);

	float clamp_sidebar_width(float requested, float window_width) const;
	float clamp_bottom_height(float requested, float available_height) const;

	void reencode_dict(tools_t::dict_t & dict, codepage_t old_cp, codepage_t new_cp);
	void decode_dict_from_codepage(tools_t::dict_t & dict, codepage_t cp);
	codepage_t active_codepage() const;
	void save_user_dict_encoded();
	void save_user_dict_as_encoded(const std::string & path);
	void save_slot_encoded(int slot_index);
	void save_slot_as_encoded(int slot_index, const std::string & path);
	void save_all_encoded();

	struct fuzzy_match_t
	{
		std::string matched_key;
		std::string matched_value;
		float similarity = 0.0f;
	};

	std::vector<fuzzy_match_t> find_fuzzy_matches(
	    const std::string & text,
	    tools_t::rec_type_t type,
	    size_t max_results = 5) const;
};
