#include "wrapping_text_widget.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>

void recalculate_layout(widget_state_t& state, const char* buf, int buf_len, float wrap_width, ImFont* font, float font_size)
{
	state.line_starts.clear();
	state.line_starts.push_back(0);

	if (buf_len == 0)
	{
		state.last_wrap_width = wrap_width;
		state.last_buf_len = buf_len;
		return;
	}

	const float scale = font_size / font->FontSize;
	const char* text_end = buf + buf_len;
	const char* pos = buf;

	while (pos < text_end)
	{
		const char* newline = static_cast<const char*>(memchr(pos, '\n', text_end - pos));
		const char* segment_end = newline ? newline : text_end;

		while (pos < segment_end)
		{
			const char* wrap_pos = font->CalcWordWrapPositionA(scale, pos, segment_end, wrap_width);

			if (wrap_pos == pos)
				wrap_pos = pos + 1;

			if (wrap_pos >= segment_end)
			{
				pos = segment_end;
				break;
			}

			while (wrap_pos < segment_end && *wrap_pos == ' ')
				++wrap_pos;

			state.line_starts.push_back(static_cast<int>(wrap_pos - buf));
			pos = wrap_pos;
		}

		if (!newline)
			break;

		pos = newline + 1;
		state.line_starts.push_back(static_cast<int>(pos - buf));
	}

	state.last_wrap_width = wrap_width;
	state.last_buf_len = buf_len;
}

int insert_text(char* buf, size_t buf_size, int cursor_pos, const char* text, int text_len)
{
	int buf_len = static_cast<int>(strlen(buf));
	int available = static_cast<int>(buf_size) - 1 - buf_len;
	int insert_len = (text_len < available) ? text_len : available;

	if (insert_len <= 0)
		return 0;

	memmove(buf + cursor_pos + insert_len, buf + cursor_pos, buf_len - cursor_pos + 1);
	memcpy(buf + cursor_pos, text, insert_len);
	return insert_len;
}

void delete_range(char* buf, int start, int end)
{
	int tail_len = static_cast<int>(strlen(buf)) - end + 1;
	memmove(buf + start, buf + end, tail_len);
}

int find_line_for_offset(const widget_state_t& state, int byte_offset)
{
	if (state.line_starts.empty())
		return 0;

	auto it = std::upper_bound(state.line_starts.begin(), state.line_starts.end(), byte_offset);
	int line = static_cast<int>(it - state.line_starts.begin()) - 1;

	if (line < 0)
		return 0;

	if (line >= static_cast<int>(state.line_starts.size()) - 1)
		return static_cast<int>(state.line_starts.size()) - 1;

	return line;
}

float get_cursor_x(const char* buf, int line_start, int cursor_pos, ImFont* font, float font_size)
{
	if (cursor_pos <= line_start)
		return 0.0f;

	return font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, buf + line_start, buf + cursor_pos).x;
}

int hit_test(const widget_state_t& state, const char* buf, float local_x, float local_y, ImFont* font, float font_size, float line_height)
{
	int buf_len = static_cast<int>(strlen(buf));
	int line_count = static_cast<int>(state.line_starts.size());

	if (line_count == 0)
		return buf_len;

	if (local_y >= line_count * line_height)
		return buf_len;

	int line_idx = static_cast<int>(local_y / line_height);
	line_idx = std::clamp(line_idx, 0, line_count - 1);

	int line_start = state.line_starts[line_idx];
	int line_end = (line_idx + 1 < line_count) ? state.line_starts[line_idx + 1] : buf_len;

	float prev_x = 0.0f;
	for (int i = line_start; i < line_end; ++i)
	{
		float cur_x = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, buf + line_start, buf + i + 1).x;
		float mid = (prev_x + cur_x) * 0.5f;

		if (local_x < mid)
			return i;

		prev_x = cur_x;
	}

	return line_end;
}

int find_prev_word_start(const char* buf, int pos)
{
	if (pos <= 0)
		return 0;

	while (pos > 0 && std::isspace(static_cast<unsigned char>(buf[pos - 1])))
		--pos;

	while (pos > 0 && !std::isspace(static_cast<unsigned char>(buf[pos - 1])))
		--pos;

	return pos;
}

int find_next_word_start(const char* buf, int buf_len, int pos)
{
	if (pos >= buf_len)
		return buf_len;

	while (pos < buf_len && !std::isspace(static_cast<unsigned char>(buf[pos])))
		++pos;

	while (pos < buf_len && std::isspace(static_cast<unsigned char>(buf[pos])))
		++pos;

	return pos;
}

bool handle_mouse_input(widget_state_t& state, const char* buf, const ImVec2& text_origin, const ImVec2& widget_size, ImFont* font, float font_size, float line_height)
{
	ImVec2 mouse_pos = ImGui::GetMousePos();
	bool inside = mouse_pos.x >= text_origin.x && mouse_pos.x < text_origin.x + widget_size.x &&
	              mouse_pos.y >= text_origin.y && mouse_pos.y < text_origin.y + widget_size.y;

	if (ImGui::IsMouseClicked(0))
	{
		if (!inside)
		{
			state.is_active = false;
			return false;
		}

		state.is_active = true;
		float local_x = mouse_pos.x - text_origin.x;
		float local_y = mouse_pos.y - text_origin.y + state.scroll_y;
		int offset = hit_test(state, buf, local_x, local_y, font, font_size, line_height);
		state.cursor_pos = offset;
		state.select_start = offset;
		state.select_end = offset;
		return false;
	}

	if (!state.is_active)
		return false;

	if (ImGui::IsMouseDragging(0))
	{
		float local_x = mouse_pos.x - text_origin.x;
		float local_y = mouse_pos.y - text_origin.y + state.scroll_y;
		int offset = hit_test(state, buf, local_x, local_y, font, font_size, line_height);
		state.select_end = offset;
		state.cursor_pos = offset;
	}

	return false;
}

static bool handle_clipboard(widget_state_t& state, char* buf, size_t buf_size)
{
	if (!state.is_active)
		return false;

	if (!ImGui::GetIO().KeyCtrl)
		return false;

	int sel_min = std::min(state.select_start, state.select_end);
	int sel_max = std::max(state.select_start, state.select_end);

	if (ImGui::IsKeyPressed(ImGuiKey_C))
	{
		if (sel_min == sel_max)
			return false;

		std::string selected(buf + sel_min, buf + sel_max);
		ImGui::SetClipboardText(selected.c_str());
		return false;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_X))
	{
		if (sel_min == sel_max)
			return false;

		std::string selected(buf + sel_min, buf + sel_max);
		ImGui::SetClipboardText(selected.c_str());
		delete_range(buf, sel_min, sel_max);
		state.cursor_pos = sel_min;
		state.select_start = sel_min;
		state.select_end = sel_min;
		return true;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_V))
	{
		const char* clip = ImGui::GetClipboardText();
		if (!clip || clip[0] == '\0')
			return false;

		if (sel_min != sel_max)
		{
			delete_range(buf, sel_min, sel_max);
			state.cursor_pos = sel_min;
		}

		int inserted = insert_text(buf, buf_size, state.cursor_pos, clip, static_cast<int>(strlen(clip)));
		state.cursor_pos += inserted;
		state.select_start = state.cursor_pos;
		state.select_end = state.cursor_pos;
		return true;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_A))
	{
		int buf_len = static_cast<int>(strlen(buf));
		state.select_start = 0;
		state.select_end = buf_len;
		state.cursor_pos = buf_len;
		return false;
	}

	return false;
}

static void handle_keyboard_navigation(widget_state_t& state, const char* buf, int buf_len, ImFont* font, float font_size)
{
	if (!state.is_active)
		return;

	bool shift = ImGui::GetIO().KeyShift;
	bool ctrl = ImGui::GetIO().KeyCtrl;
	int line_count = static_cast<int>(state.line_starts.size());

	auto begin_selection = [&]()
	{
		if (!shift)
			return;
		if (state.select_start == state.select_end)
			state.select_start = state.cursor_pos;
	};

	auto end_selection = [&]()
	{
		if (shift)
		{
			state.select_end = state.cursor_pos;
			return;
		}
		state.select_start = state.cursor_pos;
		state.select_end = state.cursor_pos;
	};

	if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
	{
		begin_selection();
		if (ctrl)
			state.cursor_pos = find_prev_word_start(buf, state.cursor_pos);
		else
			state.cursor_pos = (state.cursor_pos > 0) ? state.cursor_pos - 1 : 0;
		end_selection();
		return;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
	{
		begin_selection();
		if (ctrl)
			state.cursor_pos = find_next_word_start(buf, buf_len, state.cursor_pos);
		else
			state.cursor_pos = (state.cursor_pos < buf_len) ? state.cursor_pos + 1 : buf_len;
		end_selection();
		return;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
	{
		begin_selection();
		int current_line = find_line_for_offset(state, state.cursor_pos);
		if (current_line > 0)
		{
			int line_start = state.line_starts[current_line];
			float target_x = get_cursor_x(buf, line_start, state.cursor_pos, font, font_size);
			float line_height = font->FontSize;
			float target_y = (current_line - 1) * line_height + line_height * 0.5f;
			state.cursor_pos = hit_test(state, buf, target_x, target_y, font, font_size, line_height);
		}
		end_selection();
		return;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
	{
		begin_selection();
		int current_line = find_line_for_offset(state, state.cursor_pos);
		if (current_line < line_count - 1)
		{
			int line_start = state.line_starts[current_line];
			float target_x = get_cursor_x(buf, line_start, state.cursor_pos, font, font_size);
			float line_height = font->FontSize;
			float target_y = (current_line + 1) * line_height + line_height * 0.5f;
			state.cursor_pos = hit_test(state, buf, target_x, target_y, font, font_size, line_height);
		}
		end_selection();
		return;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Home))
	{
		begin_selection();
		int current_line = find_line_for_offset(state, state.cursor_pos);
		state.cursor_pos = state.line_starts[current_line];
		end_selection();
		return;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_End))
	{
		begin_selection();
		int current_line = find_line_for_offset(state, state.cursor_pos);
		if (current_line + 1 < line_count)
		{
			int next_start = state.line_starts[current_line + 1];
			state.cursor_pos = next_start;
			if (state.cursor_pos > 0 && buf[state.cursor_pos - 1] == '\n')
				--state.cursor_pos;
		}
		else
		{
			state.cursor_pos = buf_len;
		}
		end_selection();
		return;
	}
}

static bool handle_text_editing(widget_state_t& state, char* buf, size_t buf_size)
{
	if (!state.is_active)
		return false;

	bool modified = false;
	int buf_len = static_cast<int>(strlen(buf));

	int sel_min = std::min(state.select_start, state.select_end);
	int sel_max = std::max(state.select_start, state.select_end);
	bool has_selection = sel_min != sel_max;

	ImGuiIO& io = ImGui::GetIO();
	for (int i = 0; i < io.InputQueueCharacters.Size; ++i)
	{
		ImWchar ch = io.InputQueueCharacters[i];

		if (ch < 32 && ch != '\n')
			continue;

		if (has_selection)
		{
			delete_range(buf, sel_min, sel_max);
			state.cursor_pos = sel_min;
			buf_len = static_cast<int>(strlen(buf));
			has_selection = false;
		}

		char c = static_cast<char>(ch);
		int inserted = insert_text(buf, buf_size, state.cursor_pos, &c, 1);
		state.cursor_pos += inserted;
		buf_len = static_cast<int>(strlen(buf));
		state.select_start = state.cursor_pos;
		state.select_end = state.cursor_pos;
		modified = true;
	}

	sel_min = std::min(state.select_start, state.select_end);
	sel_max = std::max(state.select_start, state.select_end);
	has_selection = sel_min != sel_max;

	if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
	{
		if (has_selection)
		{
			delete_range(buf, sel_min, sel_max);
			state.cursor_pos = sel_min;
			state.select_start = sel_min;
			state.select_end = sel_min;
			modified = true;
		}
		else if (state.cursor_pos > 0)
		{
			delete_range(buf, state.cursor_pos - 1, state.cursor_pos);
			state.cursor_pos--;
			state.select_start = state.cursor_pos;
			state.select_end = state.cursor_pos;
			modified = true;
		}
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Delete))
	{
		buf_len = static_cast<int>(strlen(buf));
		sel_min = std::min(state.select_start, state.select_end);
		sel_max = std::max(state.select_start, state.select_end);
		has_selection = sel_min != sel_max;

		if (has_selection)
		{
			delete_range(buf, sel_min, sel_max);
			state.cursor_pos = sel_min;
			state.select_start = sel_min;
			state.select_end = sel_min;
			modified = true;
		}
		else if (state.cursor_pos < buf_len)
		{
			delete_range(buf, state.cursor_pos, state.cursor_pos + 1);
			state.select_start = state.cursor_pos;
			state.select_end = state.cursor_pos;
			modified = true;
		}
	}

	return modified;
}

static void render_widget(const widget_state_t& state, const char* buf, int buf_len, const ImVec2& text_origin, const ImVec2& widget_size, ImFont* font, float font_size, float line_height)
{
	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	ImVec2 clip_min = text_origin;
	ImVec2 clip_max = ImVec2(text_origin.x + widget_size.x, text_origin.y + widget_size.y);
	draw_list->PushClipRect(clip_min, clip_max, true);

	int line_count = static_cast<int>(state.line_starts.size());
	int sel_min = std::min(state.select_start, state.select_end);
	int sel_max = std::max(state.select_start, state.select_end);
	bool has_selection = sel_min != sel_max;

	if (has_selection)
	{
		ImU32 highlight_color = IM_COL32(51, 153, 255, 100);

		for (int i = 0; i < line_count; ++i)
		{
			int line_start = state.line_starts[i];
			int line_end = (i + 1 < line_count) ? state.line_starts[i + 1] : buf_len;

			if (sel_max <= line_start || sel_min >= line_end)
				continue;

			float line_y = text_origin.y + i * line_height - state.scroll_y;
			if (line_y + line_height < text_origin.y || line_y > clip_max.y)
				continue;

			int range_start = (sel_min > line_start) ? sel_min : line_start;
			int range_end = (sel_max < line_end) ? sel_max : line_end;

			float x0 = get_cursor_x(buf, line_start, range_start, font, font_size);
			float x1 = get_cursor_x(buf, line_start, range_end, font, font_size);

			ImVec2 rect_min = ImVec2(text_origin.x + x0, line_y);
			ImVec2 rect_max = ImVec2(text_origin.x + x1, line_y + line_height);
			draw_list->AddRectFilled(rect_min, rect_max, highlight_color);
		}
	}

	ImU32 text_color = ImGui::GetColorU32(ImGuiCol_Text);

	for (int i = 0; i < line_count; ++i)
	{
		float line_y = text_origin.y + i * line_height - state.scroll_y;

		if (line_y + line_height < text_origin.y)
			continue;
		if (line_y > clip_max.y)
			break;

		int line_start = state.line_starts[i];
		int line_end = (i + 1 < line_count) ? state.line_starts[i + 1] : buf_len;

		if (line_start < line_end)
			draw_list->AddText(font, font_size, ImVec2(text_origin.x, line_y), text_color, buf + line_start, buf + line_end);
	}

	if (state.is_active && fmodf(state.cursor_anim_timer, 1.2f) < 0.8f)
	{
		int cursor_line = find_line_for_offset(state, state.cursor_pos);
		float cursor_x = get_cursor_x(buf, state.line_starts[cursor_line], state.cursor_pos, font, font_size);
		float cursor_screen_x = text_origin.x + cursor_x;
		float cursor_screen_y = text_origin.y + cursor_line * line_height - state.scroll_y;

		ImU32 cursor_color = ImGui::GetColorU32(ImGuiCol_Text);
		draw_list->AddLine(ImVec2(cursor_screen_x, cursor_screen_y), ImVec2(cursor_screen_x, cursor_screen_y + line_height), cursor_color);
	}

	draw_list->PopClipRect();
}

void handle_scrolling(widget_state_t& state, float viewport_height, float line_height, int line_count, bool hovered)
{
	float total_height = line_count * line_height;
	float max_scroll = std::max(0.0f, total_height - viewport_height);

	if (hovered && ImGui::GetIO().MouseWheel != 0.0f)
		state.scroll_y -= ImGui::GetIO().MouseWheel * line_height * 3.0f;

	state.scroll_y = std::clamp(state.scroll_y, 0.0f, max_scroll);
}

void scroll_to_cursor(widget_state_t& state, float viewport_height, float line_height)
{
	int cursor_line = find_line_for_offset(state, state.cursor_pos);
	float cursor_top = cursor_line * line_height;
	float cursor_bottom = cursor_top + line_height;

	if (cursor_top < state.scroll_y)
		state.scroll_y = cursor_top;

	if (cursor_bottom > state.scroll_y + viewport_height)
		state.scroll_y = cursor_bottom - viewport_height;
}

void render_scrollbar(ImDrawList* draw_list, const ImVec2& widget_pos, const ImVec2& widget_size, float viewport_height, float line_height, int line_count, float scroll_y)
{
	float total_height = line_count * line_height;

	if (total_height <= viewport_height)
		return;

	float scrollbar_width = 6.0f;
	float track_x = widget_pos.x + widget_size.x - scrollbar_width;
	float track_y = widget_pos.y;
	float track_height = viewport_height;

	ImU32 track_color = IM_COL32(50, 50, 50, 100);
	draw_list->AddRectFilled(ImVec2(track_x, track_y), ImVec2(track_x + scrollbar_width, track_y + track_height), track_color);

	float thumb_height = std::max(20.0f, (viewport_height / total_height) * track_height);
	float max_scroll = total_height - viewport_height;
	float thumb_y = track_y + (scroll_y / max_scroll) * (track_height - thumb_height);

	ImU32 thumb_color = IM_COL32(150, 150, 150, 180);
	draw_list->AddRectFilled(ImVec2(track_x, thumb_y), ImVec2(track_x + scrollbar_width, thumb_y + thumb_height), thumb_color, 3.0f);
}

bool wrapping_text_widget(const char* label, char* buf, size_t buf_size, const ImVec2& size)
{
	static std::unordered_map<ImGuiID, widget_state_t> state_map;

	ImVec2 avail = ImGui::GetContentRegionAvail();
	float w = (size.x <= 0.0f) ? avail.x : size.x;
	float h = (size.y <= 0.0f) ? avail.y : size.y;

	ImGuiID id = ImGui::GetID(label);
	widget_state_t& state = state_map[id];

	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImGui::InvisibleButton(label, ImVec2(w, h));
	bool hovered = ImGui::IsItemHovered();

	ImFont* font = ImGui::GetFont();
	float font_size = ImGui::GetFontSize();
	float line_height = ImGui::GetTextLineHeight();

	int buf_len = static_cast<int>(strlen(buf));
	state.cursor_pos = std::clamp(state.cursor_pos, 0, buf_len);
	state.select_start = std::clamp(state.select_start, 0, buf_len);
	state.select_end = std::clamp(state.select_end, 0, buf_len);

	float scrollbar_width = 8.0f;
	float wrap_width = w - scrollbar_width;

	if (wrap_width != state.last_wrap_width || buf_len != state.last_buf_len)
		recalculate_layout(state, buf, buf_len, wrap_width, font, font_size);

	bool modified = false;
	handle_mouse_input(state, buf, pos, ImVec2(w, h), font, font_size, line_height);
	handle_keyboard_navigation(state, buf, buf_len, font, font_size);
	modified |= handle_clipboard(state, buf, buf_size);
	modified |= handle_text_editing(state, buf, buf_size);

	if (modified)
	{
		buf_len = static_cast<int>(strlen(buf));
		recalculate_layout(state, buf, buf_len, wrap_width, font, font_size);
	}

	int line_count = static_cast<int>(state.line_starts.size());
	scroll_to_cursor(state, h, line_height);
	handle_scrolling(state, h, line_height, line_count, hovered);

	state.cursor_anim_timer += ImGui::GetIO().DeltaTime;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	render_widget(state, buf, buf_len, pos, ImVec2(w, h), font, font_size, line_height);
	render_scrollbar(draw_list, pos, ImVec2(w, h), h, line_height, line_count, state.scroll_y);

	return modified;
}
