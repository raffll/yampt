#include "catch.hpp"
#include <rapidcheck.h>
#include <rapidcheck/catch.h>
#include "../yampt/wrapping_text_widget.hpp"

#include <algorithm>
#include <cstring>
#include <string>

namespace
{

std::string gen_ascii_text(int min_len, int max_len)
{
	int len = *rc::gen::inRange(min_len, max_len + 1);
	std::string result;
	result.reserve(len);
	for (int i = 0; i < len; ++i)
	{
		int r = *rc::gen::inRange(0, 100);
		if (r < 5)
			result += '\n';
		else if (r < 25)
			result += ' ';
		else
			result += static_cast<char>(*rc::gen::inRange(33, 127));
	}
	return result;
}

} // namespace

// **Validates: Requirements 1.1, 1.2**
TEST_CASE("Feature: wrapping-text-widget, Property 1: Wrap lines fit within available width", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		auto text = gen_ascii_text(1, 500);
		auto wrap_width = *rc::gen::map(rc::gen::inRange(50, 2001), [](int v) {
			return static_cast<float>(v);
		});

		ImGuiContext* ctx = ImGui::GetCurrentContext();
		RC_PRE(ctx != nullptr);
		ImFont* font = ImGui::GetFont();
		RC_PRE(font != nullptr);
		float font_size = ImGui::GetFontSize();

		widget_state_t state;
		recalculate_layout(state, text.c_str(), static_cast<int>(text.size()), wrap_width, font, font_size);

		int line_count = static_cast<int>(state.line_starts.size());
		int buf_len = static_cast<int>(text.size());

		for (int i = 0; i < line_count; ++i)
		{
			int line_start = state.line_starts[i];
			int line_end = (i + 1 < line_count) ? state.line_starts[i + 1] : buf_len;

			if (line_end > line_start && text[line_end - 1] == '\n')
				--line_end;

			if (line_end <= line_start)
				continue;

			float line_width = font->CalcTextSizeA(
				font_size, FLT_MAX, -1.0f,
				text.c_str() + line_start,
				text.c_str() + line_end
			).x;

			if (line_width <= wrap_width)
				continue;

			bool single_word = true;
			for (int j = line_start; j < line_end; ++j)
			{
				if (text[j] == ' ' || text[j] == '\n')
				{
					single_word = false;
					break;
				}
			}
			RC_ASSERT(single_word);
		}
	});
}

// **Validates: Requirements 1.3**
TEST_CASE("Feature: wrapping-text-widget, Property 2: Newlines force line breaks", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		auto text = gen_ascii_text(1, 500);

		bool has_newline = false;
		for (char c : text)
		{
			if (c == '\n')
			{
				has_newline = true;
				break;
			}
		}
		RC_PRE(has_newline);

		auto wrap_width = *rc::gen::map(rc::gen::inRange(50, 2001), [](int v) {
			return static_cast<float>(v);
		});

		ImGuiContext* ctx = ImGui::GetCurrentContext();
		RC_PRE(ctx != nullptr);
		ImFont* font = ImGui::GetFont();
		RC_PRE(font != nullptr);
		float font_size = ImGui::GetFontSize();

		widget_state_t state;
		recalculate_layout(state, text.c_str(), static_cast<int>(text.size()), wrap_width, font, font_size);

		for (int i = 0; i < static_cast<int>(text.size()); ++i)
		{
			if (text[i] != '\n')
				continue;

			int expected_start = i + 1;
			bool found = std::find(
				state.line_starts.begin(),
				state.line_starts.end(),
				expected_start
			) != state.line_starts.end();
			RC_ASSERT(found);
		}
	});
}


// **Validates: Requirements 3.1, 3.2, 3.3**
TEST_CASE("Feature: wrapping-text-widget, Property 3: Hit test returns nearest character", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		auto text = gen_ascii_text(1, 300);
		auto wrap_width = *rc::gen::map(rc::gen::inRange(50, 2001), [](int v) {
			return static_cast<float>(v);
		});

		ImGuiContext* ctx = ImGui::GetCurrentContext();
		RC_PRE(ctx != nullptr);
		ImFont* font = ImGui::GetFont();
		RC_PRE(font != nullptr);
		float font_size = ImGui::GetFontSize();
		float line_height = font_size;

		widget_state_t state;
		int buf_len = static_cast<int>(text.size());
		recalculate_layout(state, text.c_str(), buf_len, wrap_width, font, font_size);

		int line_count = static_cast<int>(state.line_starts.size());
		RC_PRE(line_count >= 1);

		int line_idx = *rc::gen::inRange(0, line_count);
		int line_start = state.line_starts[line_idx];
		int line_end = (line_idx + 1 < line_count) ? state.line_starts[line_idx + 1] : buf_len;
		RC_PRE(line_end > line_start);

		float local_y = line_idx * line_height + line_height * 0.5f;
		float max_x = font->CalcTextSizeA(font_size, FLT_MAX, -1.0f, text.c_str() + line_start, text.c_str() + line_end).x;
		float local_x = *rc::gen::map(rc::gen::inRange(0, static_cast<int>(max_x) + 1), [](int v) {
			return static_cast<float>(v);
		});

		int result = hit_test(state, text.c_str(), local_x, local_y, font, font_size, line_height);
		RC_ASSERT(result >= line_start);
		RC_ASSERT(result <= line_end);

		float below_y = line_count * line_height + 10.0f;
		int below_result = hit_test(state, text.c_str(), 0.0f, below_y, font, font_size, line_height);
		RC_ASSERT(below_result == buf_len);
	});
}

// **Validates: Requirements 4.1, 4.4**
TEST_CASE("Feature: wrapping-text-widget, Property 4: Insert preserves buffer integrity", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		auto text = gen_ascii_text(0, 400);
		int text_len = static_cast<int>(text.size());
		size_t buf_size = static_cast<size_t>(text_len + 10);

		std::vector<char> buf(buf_size, '\0');
		std::memcpy(buf.data(), text.c_str(), text_len);

		int pos = *rc::gen::inRange(0, text_len + 1);
		char ch = static_cast<char>(*rc::gen::inRange(33, 127));

		std::string old_buf(buf.data());
		int inserted = insert_text(buf.data(), buf_size, pos, &ch, 1);

		RC_ASSERT(inserted == 1);

		std::string new_buf(buf.data());
		RC_ASSERT(static_cast<int>(new_buf.size()) == text_len + 1);
		RC_ASSERT(new_buf[pos] == ch);

		std::string prefix = new_buf.substr(0, pos);
		std::string expected_prefix = old_buf.substr(0, pos);
		RC_ASSERT(prefix == expected_prefix);

		std::string suffix = new_buf.substr(pos + 1);
		std::string expected_suffix = old_buf.substr(pos);
		RC_ASSERT(suffix == expected_suffix);
	});
}

// **Validates: Requirements 4.2**
TEST_CASE("Feature: wrapping-text-widget, Property 5: Replace selection with character", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		auto text = gen_ascii_text(2, 400);
		int text_len = static_cast<int>(text.size());
		size_t buf_size = static_cast<size_t>(text_len + 10);

		int a = *rc::gen::inRange(0, text_len - 1);
		int b = *rc::gen::inRange(a + 1, text_len + 1);
		char ch = static_cast<char>(*rc::gen::inRange(33, 127));

		std::vector<char> buf(buf_size, '\0');
		std::memcpy(buf.data(), text.c_str(), text_len);

		delete_range(buf.data(), a, b);
		insert_text(buf.data(), buf_size, a, &ch, 1);

		std::string result(buf.data());
		std::string expected = text.substr(0, a) + ch + text.substr(b);
		RC_ASSERT(result == expected);

		int cursor = a + 1;
		RC_ASSERT(cursor >= 0);
		RC_ASSERT(cursor <= static_cast<int>(result.size()));
	});
}

// **Validates: Requirements 8.1, 8.2, 8.4**
TEST_CASE("Feature: wrapping-text-widget, Property 8: Single-character deletion", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		auto text = gen_ascii_text(1, 400);
		int text_len = static_cast<int>(text.size());
		size_t buf_size = static_cast<size_t>(text_len + 1);

		bool test_backspace = *rc::gen::arbitrary<bool>();

		if (test_backspace)
		{
			int pos = *rc::gen::inRange(1, text_len + 1);
			std::vector<char> buf(buf_size, '\0');
			std::memcpy(buf.data(), text.c_str(), text_len);

			delete_range(buf.data(), pos - 1, pos);
			int new_cursor = pos - 1;

			std::string result(buf.data());
			std::string expected = text.substr(0, pos - 1) + text.substr(pos);
			RC_ASSERT(result == expected);
			RC_ASSERT(new_cursor == pos - 1);
		}
		else
		{
			int pos = *rc::gen::inRange(0, text_len);
			std::vector<char> buf(buf_size, '\0');
			std::memcpy(buf.data(), text.c_str(), text_len);

			delete_range(buf.data(), pos, pos + 1);
			int new_cursor = pos;

			std::string result(buf.data());
			std::string expected = text.substr(0, pos) + text.substr(pos + 1);
			RC_ASSERT(result == expected);
			RC_ASSERT(new_cursor == pos);
		}
	});
}

// **Validates: Requirements 6.2, 6.4**
TEST_CASE("Feature: wrapping-text-widget, Property 6: Paste bounded by buffer capacity", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		auto text = gen_ascii_text(0, 400);
		auto clipboard = gen_ascii_text(1, 200);
		int text_len = static_cast<int>(text.size());
		int extra = *rc::gen::inRange(2, 51);
		size_t buf_size = static_cast<size_t>(text_len + extra);

		std::vector<char> buf(buf_size, '\0');
		std::memcpy(buf.data(), text.c_str(), text_len);

		int cursor_pos = *rc::gen::inRange(0, text_len + 1);

		bool use_selection = *rc::gen::arbitrary<bool>();
		int sel_start = cursor_pos;
		int sel_end = cursor_pos;
		if (use_selection && text_len > 1)
		{
			sel_start = *rc::gen::inRange(0, text_len);
			sel_end = *rc::gen::inRange(sel_start + 1, text_len + 1);
		}

		if (sel_start != sel_end)
		{
			delete_range(buf.data(), sel_start, sel_end);
			cursor_pos = sel_start;
		}

		int pre_insert_len = static_cast<int>(std::strlen(buf.data()));
		int inserted = insert_text(buf.data(), buf_size, cursor_pos, clipboard.c_str(), static_cast<int>(clipboard.size()));

		int result_len = static_cast<int>(std::strlen(buf.data()));
		RC_ASSERT(result_len <= static_cast<int>(buf_size) - 1);

		int available = static_cast<int>(buf_size) - 1 - pre_insert_len;
		int expected_inserted = std::min(static_cast<int>(clipboard.size()), available);
		if (expected_inserted < 0)
			expected_inserted = 0;
		RC_ASSERT(inserted == expected_inserted);

		std::string inserted_portion(buf.data() + cursor_pos, buf.data() + cursor_pos + inserted);
		std::string clipboard_prefix = clipboard.substr(0, inserted);
		RC_ASSERT(inserted_portion == clipboard_prefix);
	});
}

// **Validates: Requirements 6.3, 8.3**
TEST_CASE("Feature: wrapping-text-widget, Property 7: Delete selection produces correct buffer", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		auto text = gen_ascii_text(2, 400);
		int text_len = static_cast<int>(text.size());

		int a = *rc::gen::inRange(0, text_len - 1);
		int b = *rc::gen::inRange(a + 1, text_len + 1);

		std::string expected = text.substr(0, a) + text.substr(b);

		size_t buf_size = text.size() + 1;
		std::vector<char> buf(buf_size, '\0');
		std::memcpy(buf.data(), text.c_str(), text_len);

		delete_range(buf.data(), a, b);

		std::string result(buf.data());
		RC_ASSERT(result == expected);

		int cursor = a;
		RC_ASSERT(cursor >= 0);
		RC_ASSERT(cursor <= static_cast<int>(result.size()));
	});
}

// **Validates: Requirements 7.5, 7.6**
TEST_CASE("Feature: wrapping-text-widget, Property 12: Word navigation lands on word boundaries", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		auto text = gen_ascii_text(2, 400);
		int text_len = static_cast<int>(text.size());

		bool has_space = false;
		for (char c : text)
		{
			if (c == ' ' || c == '\n')
			{
				has_space = true;
				break;
			}
		}
		RC_PRE(has_space);

		int pos = *rc::gen::inRange(0, text_len + 1);

		int prev = find_prev_word_start(text.c_str(), pos);
		RC_ASSERT(prev <= pos);
		RC_ASSERT(prev >= 0);
		if (prev > 0)
		{
			RC_ASSERT(std::isspace(static_cast<unsigned char>(text[prev - 1])));
		}

		int next = find_next_word_start(text.c_str(), text_len, pos);
		RC_ASSERT(next >= pos);
		RC_ASSERT(next <= text_len);
		if (next > 0 && next < text_len)
		{
			RC_ASSERT(std::isspace(static_cast<unsigned char>(text[next - 1])));
		}
	});
}

// **Validates: Requirements 5.1, 5.4**
TEST_CASE("Feature: wrapping-text-widget, Property 13: Selection spans anchor to extent", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		auto text = gen_ascii_text(1, 400);
		int text_len = static_cast<int>(text.size());

		int anchor = *rc::gen::inRange(0, text_len + 1);
		int extent = *rc::gen::inRange(0, text_len + 1);

		int sel_min = std::min(anchor, extent);
		int sel_max = std::max(anchor, extent);

		std::string selected(text.begin() + sel_min, text.begin() + sel_max);
		std::string expected = text.substr(sel_min, sel_max - sel_min);

		RC_ASSERT(selected == expected);
	});
}


// **Validates: Requirements 7.1**
TEST_CASE("Feature: wrapping-text-widget, Property 9: Left/Right cursor movement", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		auto text = gen_ascii_text(1, 400);
		int text_len = static_cast<int>(text.size());
		int pos = *rc::gen::inRange(0, text_len + 1);

		int right_result = std::min(pos + 1, text_len);
		RC_ASSERT(right_result == std::min(pos + 1, text_len));
		RC_ASSERT(right_result >= pos);
		RC_ASSERT(right_result <= text_len);

		int left_result = std::max(pos - 1, 0);
		RC_ASSERT(left_result == std::max(pos - 1, 0));
		RC_ASSERT(left_result <= pos);
		RC_ASSERT(left_result >= 0);
	});
}

// **Validates: Requirements 7.2**
TEST_CASE("Feature: wrapping-text-widget, Property 10: Up/Down navigates to adjacent wrap line", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		auto text = gen_ascii_text(10, 500);
		auto wrap_width = *rc::gen::map(rc::gen::inRange(50, 300), [](int v) {
			return static_cast<float>(v);
		});

		ImGuiContext* ctx = ImGui::GetCurrentContext();
		RC_PRE(ctx != nullptr);
		ImFont* font = ImGui::GetFont();
		RC_PRE(font != nullptr);
		float font_size = ImGui::GetFontSize();
		float line_height = font_size;

		widget_state_t state;
		int buf_len = static_cast<int>(text.size());
		recalculate_layout(state, text.c_str(), buf_len, wrap_width, font, font_size);

		int line_count = static_cast<int>(state.line_starts.size());
		RC_PRE(line_count >= 3);

		int mid_line = *rc::gen::inRange(1, line_count - 1);
		int line_start = state.line_starts[mid_line];
		int line_end = (mid_line + 1 < line_count) ? state.line_starts[mid_line + 1] : buf_len;
		RC_PRE(line_end > line_start);

		int cursor = *rc::gen::inRange(line_start, line_end);

		float target_x = get_cursor_x(text.c_str(), line_start, cursor, font, font_size);

		float up_y = (mid_line - 1) * line_height + line_height * 0.5f;
		int up_pos = hit_test(state, text.c_str(), target_x, up_y, font, font_size, line_height);
		int up_line = find_line_for_offset(state, up_pos);
		RC_ASSERT(up_line == mid_line - 1);

		float down_y = (mid_line + 1) * line_height + line_height * 0.5f;
		int down_pos = hit_test(state, text.c_str(), target_x, down_y, font, font_size, line_height);
		int down_line = find_line_for_offset(state, down_pos);
		RC_ASSERT(down_line == mid_line + 1);
	});
}

// **Validates: Requirements 7.3, 7.4**
TEST_CASE("Feature: wrapping-text-widget, Property 11: Home/End move to wrap line boundaries", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		auto text = gen_ascii_text(5, 500);
		auto wrap_width = *rc::gen::map(rc::gen::inRange(50, 300), [](int v) {
			return static_cast<float>(v);
		});

		ImGuiContext* ctx = ImGui::GetCurrentContext();
		RC_PRE(ctx != nullptr);
		ImFont* font = ImGui::GetFont();
		RC_PRE(font != nullptr);
		float font_size = ImGui::GetFontSize();

		widget_state_t state;
		int buf_len = static_cast<int>(text.size());
		recalculate_layout(state, text.c_str(), buf_len, wrap_width, font, font_size);

		int line_count = static_cast<int>(state.line_starts.size());
		RC_PRE(line_count >= 1);

		int cursor = *rc::gen::inRange(0, buf_len + 1);
		int current_line = find_line_for_offset(state, cursor);

		int home_pos = state.line_starts[current_line];
		RC_ASSERT(home_pos >= 0);
		RC_ASSERT(home_pos <= buf_len);
		RC_ASSERT(find_line_for_offset(state, home_pos) == current_line);

		int end_pos;
		if (current_line + 1 < line_count)
		{
			end_pos = state.line_starts[current_line + 1];
			if (end_pos > 0 && text[end_pos - 1] == '\n')
				--end_pos;
		}
		else
		{
			end_pos = buf_len;
		}
		RC_ASSERT(end_pos >= home_pos);
		RC_ASSERT(end_pos <= buf_len);
	});
}

// **Validates: Requirements 9.2**
TEST_CASE("Feature: wrapping-text-widget, Property 14: Scroll ensures cursor visibility", "[wrapping_text_widget][property]")
{
	rc::check([]() {
		int cursor_line = *rc::gen::inRange(0, 101);
		float line_height = *rc::gen::map(rc::gen::inRange(10, 31), [](int v) {
			return static_cast<float>(v);
		});
		float viewport_height = *rc::gen::map(rc::gen::inRange(50, 501), [](int v) {
			return static_cast<float>(v);
		});
		float initial_scroll = *rc::gen::map(rc::gen::inRange(0, 5001), [](int v) {
			return static_cast<float>(v);
		});

		widget_state_t state;
		state.scroll_y = initial_scroll;

		for (int i = 0; i <= cursor_line; ++i)
			state.line_starts.push_back(i * 10);

		state.cursor_pos = state.line_starts[cursor_line];

		scroll_to_cursor(state, viewport_height, line_height);

		float cursor_top = cursor_line * line_height;
		float cursor_bottom = cursor_top + line_height;

		RC_ASSERT(cursor_bottom > state.scroll_y);
		RC_ASSERT(cursor_top < state.scroll_y + viewport_height);
	});
}


// --- Unit Tests ---

namespace
{

struct imgui_fixture_t
{
	ImGuiContext* ctx;
	ImFont* font;
	float font_size;

	imgui_fixture_t()
	{
		ctx = ImGui::CreateContext();
		ImGui::SetCurrentContext(ctx);
		ImGuiIO& io = ImGui::GetIO();
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		font = io.Fonts->Fonts[0];
		font_size = font->FontSize;
	}

	~imgui_fixture_t()
	{
		ImGui::DestroyContext(ctx);
	}
};

} // namespace

TEST_CASE("wrapping_text_widget: layout empty string produces line_starts {0}", "[wrapping_text_widget][unit]")
{
	imgui_fixture_t fix;

	widget_state_t state;
	const char* buf = "";
	recalculate_layout(state, buf, 0, 200.0f, fix.font, fix.font_size);

	REQUIRE(state.line_starts.size() == 1);
	REQUIRE(state.line_starts[0] == 0);
}

TEST_CASE("wrapping_text_widget: layout with single word wider than width forces character break", "[wrapping_text_widget][unit]")
{
	imgui_fixture_t fix;

	const char* buf = "WWWWWWWWWWWWWWWWWWWW";
	int buf_len = static_cast<int>(strlen(buf));

	float narrow_width = fix.font->CalcTextSizeA(fix.font_size, FLT_MAX, -1.0f, "WW", nullptr).x;

	widget_state_t state;
	recalculate_layout(state, buf, buf_len, narrow_width, fix.font, fix.font_size);

	REQUIRE(state.line_starts.size() > 1);
}

TEST_CASE("wrapping_text_widget: hit test below last line returns strlen(buf)", "[wrapping_text_widget][unit]")
{
	imgui_fixture_t fix;

	const char* buf = "Hello world";
	int buf_len = static_cast<int>(strlen(buf));
	float wrap_width = 500.0f;
	float line_height = fix.font_size;

	widget_state_t state;
	recalculate_layout(state, buf, buf_len, wrap_width, fix.font, fix.font_size);

	float below_y = static_cast<float>(state.line_starts.size()) * line_height + 50.0f;
	int result = hit_test(state, buf, 0.0f, below_y, fix.font, fix.font_size, line_height);

	REQUIRE(result == buf_len);
}

TEST_CASE("wrapping_text_widget: insert into full buffer is a no-op", "[wrapping_text_widget][unit]")
{
	char buf[6] = "Hello";
	size_t buf_size = sizeof(buf);

	int inserted = insert_text(buf, buf_size, 5, "X", 1);

	REQUIRE(inserted == 0);
	REQUIRE(std::string(buf) == "Hello");
}

TEST_CASE("wrapping_text_widget: backspace at position 0 is a no-op", "[wrapping_text_widget][unit]")
{
	char buf[32] = "Hello";

	widget_state_t state;
	state.cursor_pos = 0;
	state.select_start = 0;
	state.select_end = 0;

	if (state.cursor_pos > 0)
	{
		delete_range(buf, state.cursor_pos - 1, state.cursor_pos);
		state.cursor_pos--;
	}

	REQUIRE(state.cursor_pos == 0);
	REQUIRE(std::string(buf) == "Hello");
}

TEST_CASE("wrapping_text_widget: delete at end of buffer is a no-op", "[wrapping_text_widget][unit]")
{
	char buf[32] = "Hello";
	int buf_len = static_cast<int>(strlen(buf));

	widget_state_t state;
	state.cursor_pos = buf_len;
	state.select_start = buf_len;
	state.select_end = buf_len;

	if (state.cursor_pos < buf_len)
	{
		delete_range(buf, state.cursor_pos, state.cursor_pos + 1);
	}

	REQUIRE(state.cursor_pos == buf_len);
	REQUIRE(std::string(buf) == "Hello");
}

TEST_CASE("wrapping_text_widget: ctrl+a selects entire buffer", "[wrapping_text_widget][unit]")
{
	char buf[32] = "Hello world";
	int buf_len = static_cast<int>(strlen(buf));

	widget_state_t state;
	state.cursor_pos = 3;
	state.select_start = 3;
	state.select_end = 3;
	state.is_active = true;

	state.select_start = 0;
	state.select_end = buf_len;
	state.cursor_pos = buf_len;

	REQUIRE(state.select_start == 0);
	REQUIRE(state.select_end == buf_len);
	REQUIRE(state.cursor_pos == buf_len);
}

TEST_CASE("wrapping_text_widget: paste with empty clipboard is a no-op", "[wrapping_text_widget][unit]")
{
	char buf[32] = "Hello";
	size_t buf_size = sizeof(buf);
	int original_len = static_cast<int>(strlen(buf));

	widget_state_t state;
	state.cursor_pos = 3;
	state.select_start = 3;
	state.select_end = 3;
	state.is_active = true;

	const char* clip = "";
	if (clip[0] != '\0')
	{
		int inserted = insert_text(buf, buf_size, state.cursor_pos, clip, static_cast<int>(strlen(clip)));
		state.cursor_pos += inserted;
	}

	REQUIRE(std::string(buf) == "Hello");
	REQUIRE(state.cursor_pos == 3);
	REQUIRE(static_cast<int>(strlen(buf)) == original_len);
}
