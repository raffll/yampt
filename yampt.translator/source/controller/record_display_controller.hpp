#pragma once

#include "../editor/byte_limit_validator.hpp"
#include "../editor/edit_history.hpp"
#include "../editor/glossary.hpp"
#include "../highlighter/grammar_checker.hpp"
#include "../highlighter/highlight_applier.hpp"
#include "../highlighter/highlight_coordinator.hpp"
#include "../model/record_table_model.hpp"
#include "../model/table_row.hpp"
#include "editor_controller.hpp"
#include <QAction>
#include <QTextEdit>

class annotations_view_t;
class book_preview_view_t;
class dict_document_t;
class document_t;
class editor_view_t;
class history_view_t;
class translation_suggestion_view_t;
class validation_view_t;

struct record_display_deps_t
{
	editor_view_t & editor_view;
	record_table_model_t & table_model;
	editor_controller_t & editor_controller;
	glossary_t & glossary;
	grammar_checker_t & grammar_checker;
	byte_limit_validator_t & byte_limit_validator;
	edit_history_t & edit_history;
	annotations_view_t & annotations_view;
	history_view_t & history_view;
	book_preview_view_t & book_preview_view;
	validation_view_t & validation_view;
	translation_suggestion_view_t & translation_suggestion_view;
	extra_selections_state_t & extra_sel_original;
	extra_selections_state_t & extra_sel_adapted;
	extra_selections_state_t & extra_sel_translation;
	QAction & grammar_check_action;
};

class record_display_controller_t
{
public:
	explicit record_display_controller_t(record_display_deps_t deps);

	void load_record(int row, document_t * active_doc);
	void apply_translation_highlights(const table_row_t * row_data);
	void update_validation();
	void update_annotations(document_t * active_doc);

private:
	void load_record_clear();
	void load_record_script(const table_row_t * row_data);
	void load_record_plain(const table_row_t * row_data);

	record_display_deps_t m_deps;
};
