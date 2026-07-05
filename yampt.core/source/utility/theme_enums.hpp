#pragma once

enum class theme_t
{
	light,
	dark
};

enum class color_name_t
{
	window_background,
	window_text,
	editor_background,
	editor_text,
	selection_background,
	selection_text,
	disabled_text,

	status_translated,
	status_untranslated,
	status_missing,
	status_duplicate,
	status_mismatch,
	status_error,
	status_reused,
	status_adapted,
	status_changed,
	status_outdated,
	status_in_progress,
	status_model,
	status_propagated,
	status_heuristic,
	status_to_verify,
	status_ambiguous,

	syntax_function,
	syntax_comment,
	syntax_string,
	syntax_html_tag,
	syntax_forbidden_background,
	syntax_hyperlink,
	syntax_misspelled,

	diff_added_background,
	diff_removed_background,
	diff_changed_background,

	annotation_dial_topic,
	annotation_glossary_term,

	conflict_all_no_conflict_raw,
	conflict_all_override_benign_raw,
	conflict_all_conflict_raw,
	conflict_this_master,
	conflict_this_identical,
	conflict_this_override_wins,
	conflict_this_conflict_wins,
	conflict_this_conflict_loses,
	conflict_this_deleted,

	color_name_count
};
