#pragma once

#include "theme_enums.hpp"
#include <array>
#include <cstdint>

struct color_rgb_t
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha = 255;
};

inline constexpr auto light_palette = []()
{
	std::array<color_rgb_t, static_cast<size_t>(color_name_t::color_name_count)> palette {};

	palette[static_cast<size_t>(color_name_t::window_background)] = { 255, 255, 255 };
	palette[static_cast<size_t>(color_name_t::window_text)] = { 0, 0, 0 };
	palette[static_cast<size_t>(color_name_t::editor_background)] = { 255, 255, 255 };
	palette[static_cast<size_t>(color_name_t::editor_text)] = { 0, 0, 0 };
	palette[static_cast<size_t>(color_name_t::selection_background)] = { 51, 153, 255 };
	palette[static_cast<size_t>(color_name_t::selection_text)] = { 255, 255, 255 };
	palette[static_cast<size_t>(color_name_t::disabled_text)] = { 128, 128, 128 };

	palette[static_cast<size_t>(color_name_t::status_translated)] = { 128, 230, 128 };
	palette[static_cast<size_t>(color_name_t::status_untranslated)] = { 166, 166, 166 };
	palette[static_cast<size_t>(color_name_t::status_missing)] = { 242, 140, 89 };
	palette[static_cast<size_t>(color_name_t::status_duplicate)] = { 242, 230, 102 };
	palette[static_cast<size_t>(color_name_t::status_mismatch)] = { 230, 115, 140 };
	palette[static_cast<size_t>(color_name_t::status_error)] = { 242, 102, 102 };
	palette[static_cast<size_t>(color_name_t::status_reused)] = { 128, 217, 179 };
	palette[static_cast<size_t>(color_name_t::status_adapted)] = { 179, 140, 217 };
	palette[static_cast<size_t>(color_name_t::status_changed)] = { 242, 179, 102 };
	palette[static_cast<size_t>(color_name_t::status_outdated)] = { 220, 140, 80 };
	palette[static_cast<size_t>(color_name_t::status_in_progress)] = { 102, 153, 242 };
	palette[static_cast<size_t>(color_name_t::status_model)] = { 100, 180, 220 };
	palette[static_cast<size_t>(color_name_t::status_propagated)] = { 180, 230, 230 };
	palette[static_cast<size_t>(color_name_t::status_heuristic)] = { 100, 180, 160 };
	palette[static_cast<size_t>(color_name_t::status_to_verify)] = { 180, 200, 180 };
	palette[static_cast<size_t>(color_name_t::status_ambiguous)] = { 230, 180, 60 };

	palette[static_cast<size_t>(color_name_t::syntax_function)] = { 100, 180, 255 };
	palette[static_cast<size_t>(color_name_t::syntax_comment)] = { 128, 128, 128 };
	palette[static_cast<size_t>(color_name_t::syntax_string)] = { 200, 150, 50 };
	palette[static_cast<size_t>(color_name_t::syntax_html_tag)] = { 100, 0, 20 };
	palette[static_cast<size_t>(color_name_t::syntax_forbidden_background)] = { 255, 200, 180 };
	palette[static_cast<size_t>(color_name_t::syntax_hyperlink)] = { 70, 130, 220 };
	palette[static_cast<size_t>(color_name_t::syntax_misspelled)] = { 220, 50, 50 };

	palette[static_cast<size_t>(color_name_t::diff_added_background)] = { 200, 240, 200 };
	palette[static_cast<size_t>(color_name_t::diff_removed_background)] = { 255, 200, 200 };
	palette[static_cast<size_t>(color_name_t::diff_changed_background)] = { 255, 200, 130 };

	palette[static_cast<size_t>(color_name_t::annotation_dial_topic)] = { 70, 130, 200, 60 };
	palette[static_cast<size_t>(color_name_t::annotation_glossary_term)] = { 70, 180, 70, 60 };

	palette[static_cast<size_t>(color_name_t::conflict_all_no_conflict_raw)] = { 0, 255, 0 };
	palette[static_cast<size_t>(color_name_t::conflict_all_override_benign_raw)] = { 255, 255, 0 };
	palette[static_cast<size_t>(color_name_t::conflict_all_conflict_raw)] = { 255, 0, 0 };
	palette[static_cast<size_t>(color_name_t::conflict_this_master)] = { 128, 0, 128 };
	palette[static_cast<size_t>(color_name_t::conflict_this_identical)] = { 128, 128, 128 };
	palette[static_cast<size_t>(color_name_t::conflict_this_override_wins)] = { 0, 128, 0 };
	palette[static_cast<size_t>(color_name_t::conflict_this_conflict_wins)] = { 255, 128, 64 };
	palette[static_cast<size_t>(color_name_t::conflict_this_conflict_loses)] = { 255, 0, 0 };
	palette[static_cast<size_t>(color_name_t::conflict_this_deleted)] = { 100, 100, 100 };

	return palette;
}();

inline constexpr auto dark_palette = []()
{
	std::array<color_rgb_t, static_cast<size_t>(color_name_t::color_name_count)> palette {};

	palette[static_cast<size_t>(color_name_t::window_background)] = { 30, 30, 30 };
	palette[static_cast<size_t>(color_name_t::window_text)] = { 220, 220, 220 };
	palette[static_cast<size_t>(color_name_t::editor_background)] = { 35, 35, 40 };
	palette[static_cast<size_t>(color_name_t::editor_text)] = { 210, 210, 210 };
	palette[static_cast<size_t>(color_name_t::selection_background)] = { 38, 79, 120 };
	palette[static_cast<size_t>(color_name_t::selection_text)] = { 255, 255, 255 };
	palette[static_cast<size_t>(color_name_t::disabled_text)] = { 100, 100, 100 };

	palette[static_cast<size_t>(color_name_t::status_translated)] = { 80, 180, 80 };
	palette[static_cast<size_t>(color_name_t::status_untranslated)] = { 140, 140, 140 };
	palette[static_cast<size_t>(color_name_t::status_missing)] = { 200, 120, 70 };
	palette[static_cast<size_t>(color_name_t::status_duplicate)] = { 200, 190, 80 };
	palette[static_cast<size_t>(color_name_t::status_mismatch)] = { 190, 90, 115 };
	palette[static_cast<size_t>(color_name_t::status_error)] = { 200, 80, 80 };
	palette[static_cast<size_t>(color_name_t::status_reused)] = { 80, 180, 140 };
	palette[static_cast<size_t>(color_name_t::status_adapted)] = { 150, 110, 190 };
	palette[static_cast<size_t>(color_name_t::status_changed)] = { 200, 150, 80 };
	palette[static_cast<size_t>(color_name_t::status_outdated)] = { 180, 115, 65 };
	palette[static_cast<size_t>(color_name_t::status_in_progress)] = { 80, 130, 200 };
	palette[static_cast<size_t>(color_name_t::status_model)] = { 80, 155, 190 };
	palette[static_cast<size_t>(color_name_t::status_propagated)] = { 140, 190, 190 };
	palette[static_cast<size_t>(color_name_t::status_heuristic)] = { 80, 150, 130 };
	palette[static_cast<size_t>(color_name_t::status_to_verify)] = { 140, 165, 140 };
	palette[static_cast<size_t>(color_name_t::status_ambiguous)] = { 190, 150, 50 };

	palette[static_cast<size_t>(color_name_t::syntax_function)] = { 120, 200, 255 };
	palette[static_cast<size_t>(color_name_t::syntax_comment)] = { 150, 150, 150 };
	palette[static_cast<size_t>(color_name_t::syntax_string)] = { 230, 180, 80 };
	palette[static_cast<size_t>(color_name_t::syntax_html_tag)] = { 200, 130, 150 };
	palette[static_cast<size_t>(color_name_t::syntax_forbidden_background)] = { 120, 50, 50 };
	palette[static_cast<size_t>(color_name_t::syntax_hyperlink)] = { 100, 160, 240 };
	palette[static_cast<size_t>(color_name_t::syntax_misspelled)] = { 240, 80, 80 };

	palette[static_cast<size_t>(color_name_t::diff_added_background)] = { 80, 180, 80, 80 };
	palette[static_cast<size_t>(color_name_t::diff_removed_background)] = { 200, 80, 80, 80 };
	palette[static_cast<size_t>(color_name_t::diff_changed_background)] = { 180, 130, 60, 100 };

	palette[static_cast<size_t>(color_name_t::annotation_dial_topic)] = { 40, 55, 75 };
	palette[static_cast<size_t>(color_name_t::annotation_glossary_term)] = { 35, 60, 40 };

	palette[static_cast<size_t>(color_name_t::conflict_all_no_conflict_raw)] = { 0, 180, 0 };
	palette[static_cast<size_t>(color_name_t::conflict_all_override_benign_raw)] = { 180, 180, 0 };
	palette[static_cast<size_t>(color_name_t::conflict_all_conflict_raw)] = { 180, 0, 0 };
	palette[static_cast<size_t>(color_name_t::conflict_this_master)] = { 180, 80, 180 };
	palette[static_cast<size_t>(color_name_t::conflict_this_identical)] = { 150, 150, 150 };
	palette[static_cast<size_t>(color_name_t::conflict_this_override_wins)] = { 80, 200, 80 };
	palette[static_cast<size_t>(color_name_t::conflict_this_conflict_wins)] = { 255, 165, 100 };
	palette[static_cast<size_t>(color_name_t::conflict_this_conflict_loses)] = { 255, 80, 80 };
	palette[static_cast<size_t>(color_name_t::conflict_this_deleted)] = { 140, 140, 140 };

	return palette;
}();
