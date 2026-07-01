#pragma once

#include "translation_edit_view.hpp"
#include <optional>
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
	    const std::string & adapted_from,
	    bool use_original = false);
	void clear_details();

	void load_script_entry(const std::string & old_text, const std::string & new_text);
	std::string reconstruct_script_line() const;
	bool has_script_template() const;
	size_t script_slot_count() const;
	void clear_script_template();

	void set_split_ratio(double ratio);
	double get_split_ratio() const;

signals:
	void text_changed();
	void apply_clicked();

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

	std::optional<script_template_t> m_script_template;
};
