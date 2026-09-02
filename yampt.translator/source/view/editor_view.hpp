#pragma once

#include "../highlighter/highlight_coordinator.hpp"
#include "translation_edit_view.hpp"
#include <optional>
#include <set>
#include <string>
#include <vector>
#include <QTextEdit>
#include <QWidget>

struct script_template_t
{
	std::string full_line;
	std::vector<size_t> quote_starts;
	std::vector<size_t> quote_ends;
};

class QLabel;
class QPushButton;
class QSplitter;

class editor_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit editor_view_t(QWidget * parent = nullptr);

	translation_edit_view_t * original_view() const;
	translation_edit_view_t * details_view() const;
	translation_edit_view_t * translation_editor() const;

	void set_details(const std::string & text);
	QList<QTextEdit::ExtraSelection> highlight_adapted_diff(
	    const std::string & new_text,
	    const std::string & adapted_from);
	void clear_details();

	void load_script_entry(const std::string & old_text, const std::string & new_text);
	std::string reconstruct_script_line() const;
	bool has_script_template() const;
	size_t script_slot_count() const;
	void clear_script_template();

	void set_split_ratio(double ratio);
	double get_split_ratio() const;
	void set_scroll_sync(bool enabled);

	std::set<highlight_kind_t> enabled_highlight_kinds() const;
	void set_enabled_highlight_kinds(const std::set<highlight_kind_t> & kinds);

	void set_spell_check_checked(bool checked);
	void set_grammar_check_checked(bool checked);
	void set_whitespace_checked(bool checked);

signals:
	void text_changed();
	void apply_clicked();
	void highlight_filter_changed();
	void spell_check_toggled(bool checked);
	void grammar_check_toggled(bool checked);
	void whitespace_toggled(bool checked);

private:
	QWidget * setup_left_panel(QSplitter * parent_splitter);
	QWidget * setup_right_panel(QSplitter * parent_splitter);
	void setup_connections();

	static std::vector<std::string> extract_quoted_strings(const std::string & source_text);
	static QString join_extracted_lines(const std::vector<std::string> & extracted);

	QSplitter * m_splitter = nullptr;
	translation_edit_view_t * m_original_view = nullptr;
	translation_edit_view_t * m_adapted_from_view = nullptr;
	QWidget * m_adapted_from_container = nullptr;
	QPushButton * m_adapted_toggle = nullptr;
	translation_edit_view_t * m_translation_editor = nullptr;
	QLabel * m_original_label = nullptr;
	QLabel * m_translation_label = nullptr;
	QPushButton * m_apply_button = nullptr;
	QPushButton * m_hyperlink_toggle = nullptr;
	QPushButton * m_inflection_toggle = nullptr;
	QPushButton * m_glossary_toggle = nullptr;
	QPushButton * m_spell_toggle = nullptr;
	QPushButton * m_grammar_toggle = nullptr;
	QPushButton * m_whitespace_toggle = nullptr;

	std::optional<script_template_t> m_script_template;
	bool m_scroll_syncing = false;
	bool m_scroll_sync_enabled = true;
};
