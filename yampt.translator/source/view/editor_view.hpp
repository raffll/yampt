#pragma once

#include "translation_edit_view.hpp"

#include <QTextEdit>
#include <QWidget>
#include <optional>
#include <string>
#include <vector>

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

	QSplitter * splitter_ = nullptr;
	translation_edit_view_t * original_view_ = nullptr;
	translation_edit_view_t * adapted_from_view_ = nullptr;
	QWidget * adapted_from_container_ = nullptr;
	QPushButton * adapted_toggle_ = nullptr;
	translation_edit_view_t * translation_editor_ = nullptr;
	QLabel * original_label_ = nullptr;
	QLabel * translation_label_ = nullptr;
	QPushButton * apply_button_ = nullptr;

	std::optional<script_template_t> script_template_;
};
