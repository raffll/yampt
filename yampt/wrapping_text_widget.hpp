#pragma once

#include "imgui.h"
#include <vector>

struct widget_state_t
{
	int cursor_pos = 0;
	int select_start = 0;
	int select_end = 0;
	float scroll_y = 0.0f;
	std::vector<int> line_starts;
	float last_wrap_width = 0.0f;
	int last_buf_len = 0;
	float cursor_anim_timer = 0.0f;
	bool is_active = false;
	int prev_cursor_pos = -1;
};

bool wrapping_text_widget(const char* label, char* buf, size_t buf_size, const ImVec2& size = ImVec2(0, 0));
void reset_wrapping_text_widget(const char* label);

void recalculate_layout(widget_state_t& state, const char* buf, int buf_len, float wrap_width, ImFont* font, float font_size);
int find_line_for_offset(const widget_state_t& state, int byte_offset);
float get_cursor_x(const char* buf, int line_start, int cursor_pos, ImFont* font, float font_size);
int hit_test(const widget_state_t& state, const char* buf, float local_x, float local_y, ImFont* font, float font_size, float line_height);
int insert_text(char* buf, size_t buf_size, int cursor_pos, const char* text, int text_len);
void delete_range(char* buf, int start, int end);
int find_prev_word_start(const char* buf, int pos);
int find_next_word_start(const char* buf, int buf_len, int pos);
bool handle_mouse_input(widget_state_t& state, const char* buf, const ImVec2& text_origin, const ImVec2& widget_size, ImFont* font, float font_size, float line_height);
void handle_scrolling(widget_state_t& state, float viewport_height, float line_height, int line_count, bool hovered);
void scroll_to_cursor(widget_state_t& state, float viewport_height, float line_height);
void render_scrollbar(ImDrawList* draw_list, const ImVec2& widget_pos, const ImVec2& widget_size, float viewport_height, float line_height, int line_count, float scroll_y);
