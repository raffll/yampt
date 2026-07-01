#include "editor_view.hpp"
#include "line_number_gutter.hpp"
#include <utility/char_diff.hpp>
#include <algorithm>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QTextEdit>
#include <QVBoxLayout>

editor_view_t::editor_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_splitter = new QSplitter(Qt::Horizontal, this);
	m_splitter->addWidget(setup_left_panel(m_splitter));
	m_splitter->addWidget(setup_right_panel(m_splitter));
	m_splitter->setSizes({ 500, 500 });

	QFont editor_font("Segoe UI", 10);
	m_original_view->setFont(editor_font);
	m_adapted_from_view->setFont(editor_font);
	m_translation_editor->setFont(editor_font);

	layout->addWidget(m_splitter);
	setup_connections();
}

QWidget * editor_view_t::setup_left_panel(QSplitter * parent_splitter)
{
	auto * left_widget = new QWidget(parent_splitter);
	auto * left_layout = new QVBoxLayout(left_widget);
	left_layout->setContentsMargins(0, 0, 0, 0);

	m_original_label = new QLabel("Original", left_widget);
	m_original_label->setAlignment(Qt::AlignCenter);

	m_original_view = new translation_edit_view_t(left_widget);
	m_original_view->setReadOnly(true);
	auto palette = m_original_view->palette();
	palette.setColor(QPalette::Base, QColor(245, 245, 245));
	m_original_view->setPalette(palette);

	auto * original_container = new QWidget(left_widget);
	auto * original_hlayout = new QHBoxLayout(original_container);
	original_hlayout->setContentsMargins(0, 0, 0, 0);
	original_hlayout->setSpacing(0);
	original_hlayout->addWidget(new line_number_gutter_t(m_original_view, original_container));
	original_hlayout->addWidget(m_original_view);

	m_adapted_from_view = new translation_edit_view_t(left_widget);
	m_adapted_from_view->setReadOnly(true);
	auto adapted_palette = m_adapted_from_view->palette();
	adapted_palette.setColor(QPalette::Base, QColor(240, 235, 250));
	m_adapted_from_view->setPalette(adapted_palette);

	m_adapted_from_container = new QWidget(left_widget);
	auto * adapted_hlayout = new QHBoxLayout(m_adapted_from_container);
	adapted_hlayout->setContentsMargins(0, 0, 0, 0);
	adapted_hlayout->setSpacing(0);
	adapted_hlayout->addWidget(new line_number_gutter_t(m_adapted_from_view, m_adapted_from_container));
	adapted_hlayout->addWidget(m_adapted_from_view);
	m_adapted_from_container->setVisible(false);

	m_adapted_toggle = new QPushButton("Details", left_widget);
	m_adapted_toggle->setCheckable(true);
	m_adapted_toggle->setChecked(true);
	m_adapted_toggle->setToolTip("Show/hide adapted from panel");
	m_adapted_toggle->setVisible(false);

	left_layout->addWidget(m_original_label);
	left_layout->addWidget(original_container);
	left_layout->addWidget(m_adapted_from_container);
	left_layout->addWidget(m_adapted_toggle);

	return left_widget;
}

QWidget * editor_view_t::setup_right_panel(QSplitter * parent_splitter)
{
	auto * right_widget = new QWidget(parent_splitter);
	auto * right_layout = new QVBoxLayout(right_widget);
	right_layout->setContentsMargins(0, 0, 0, 0);

	m_translation_label = new QLabel("Translation", right_widget);
	m_translation_label->setAlignment(Qt::AlignCenter);

	m_translation_editor = new translation_edit_view_t(right_widget);

	m_apply_button = new QPushButton("Next (Shift+Enter)", right_widget);
	m_apply_button->setToolTip("Apply changes and move to next entry");

	auto * translation_container = new QWidget(right_widget);
	auto * translation_hlayout = new QHBoxLayout(translation_container);
	translation_hlayout->setContentsMargins(0, 0, 0, 0);
	translation_hlayout->setSpacing(0);
	translation_hlayout->addWidget(new line_number_gutter_t(m_translation_editor, translation_container));
	translation_hlayout->addWidget(m_translation_editor);

	right_layout->addWidget(m_translation_label);
	right_layout->addWidget(translation_container);
	right_layout->addWidget(m_apply_button);

	return right_widget;
}

void editor_view_t::setup_connections()
{
	connect(
	    m_adapted_toggle,
	    &QPushButton::toggled,
	    this,
	    [this](bool checked) { m_adapted_from_container->setVisible(checked); });

	connect(m_translation_editor, &QPlainTextEdit::textChanged, this, &editor_view_t::text_changed);
	connect(m_apply_button, &QPushButton::clicked, this, &editor_view_t::apply_clicked);
}

translation_edit_view_t * editor_view_t::original_view() const
{
	return m_original_view;
}

translation_edit_view_t * editor_view_t::details_view() const
{
	return m_adapted_from_view;
}

translation_edit_view_t * editor_view_t::translation_editor() const
{
	return m_translation_editor;
}

void editor_view_t::set_split_ratio(double ratio)
{
	ratio = std::clamp(ratio, 0.2, 0.8);
	const int total = m_splitter->width();
	const int left = static_cast<int>(total * ratio);
	m_splitter->setSizes({ left, total - left });
}

double editor_view_t::get_split_ratio() const
{
	const auto sizes = m_splitter->sizes();
	const int total = sizes[0] + sizes[1];
	if (total == 0)
		return 0.5;

	return static_cast<double>(sizes[0]) / total;
}

void editor_view_t::set_details(const std::string & text)
{
	auto display = QString::fromStdString(text);
	display.replace('|', '\n');
	m_adapted_from_view->setPlainText(display);
	m_adapted_toggle->setVisible(true);
	if (m_adapted_toggle->isChecked())
	{
		m_adapted_from_container->setVisible(true);
	}
}

QList<QTextEdit::ExtraSelection> editor_view_t::highlight_adapted_diff(
    const std::string & new_text,
    const std::string & adapted_from,
    bool /*use_original*/)
{
	QList<QTextEdit::ExtraSelection> selections;

	auto first_segment = adapted_from;
	std::string remainder;
	const auto pipe_pos = adapted_from.find('|');
	if (pipe_pos != std::string::npos)
	{
		first_segment = adapted_from.substr(0, pipe_pos);
		remainder = adapted_from.substr(pipe_pos);
	}

	const auto segments = compute_char_diff(first_segment, new_text);

	std::string merged_text;
	std::vector<std::pair<int, int>> inserted_ranges;
	std::vector<std::pair<int, int>> deleted_ranges;

	for (const auto & segment : segments)
	{
		const int start = static_cast<int>(merged_text.size());
		merged_text += segment.text;
		const int end = static_cast<int>(merged_text.size());

		if (segment.operation == diff_op_t::inserted)
			inserted_ranges.emplace_back(start, end);
		else if (segment.operation == diff_op_t::deleted)
			deleted_ranges.emplace_back(start, end);
	}

	if (!remainder.empty())
	{
		auto remainder_display = remainder;
		for (auto & ch : remainder_display)
		{
			if (ch == '|')
				ch = '\n';
		}
		merged_text += remainder_display;
	}

	m_adapted_from_view->setPlainText(QString::fromStdString(merged_text));

	QTextCharFormat inserted_format;
	inserted_format.setBackground(QColor(180, 255, 180));

	QTextCharFormat deleted_format;
	deleted_format.setBackground(QColor(255, 180, 180));
	deleted_format.setFontStrikeOut(true);

	for (const auto & [start, end] : inserted_ranges)
	{
		QTextEdit::ExtraSelection sel;
		sel.format = inserted_format;
		sel.cursor = m_adapted_from_view->textCursor();
		sel.cursor.setPosition(start);
		sel.cursor.setPosition(end, QTextCursor::KeepAnchor);
		selections.append(sel);
	}

	for (const auto & [start, end] : deleted_ranges)
	{
		QTextEdit::ExtraSelection sel;
		sel.format = deleted_format;
		sel.cursor = m_adapted_from_view->textCursor();
		sel.cursor.setPosition(start);
		sel.cursor.setPosition(end, QTextCursor::KeepAnchor);
		selections.append(sel);
	}

	return selections;
}

void editor_view_t::clear_details()
{
	m_adapted_from_view->clear();
	m_adapted_from_container->setVisible(false);
	m_adapted_toggle->setVisible(false);
}

std::vector<std::string> editor_view_t::extract_quoted_strings(const std::string & source_text)
{
	std::vector<std::string> result;
	size_t pos = 0;

	while (pos < source_text.size())
	{
		auto i = source_text.find('"', pos);
		if (i == std::string::npos)
			break;

		auto j = source_text.find('"', i + 1);
		if (j == std::string::npos)
			break;

		result.push_back(source_text.substr(i + 1, j - i - 1));
		pos = j + 1;
	}

	return result;
}

QString editor_view_t::join_extracted_lines(const std::vector<std::string> & extracted)
{
	QString joined;
	for (size_t k = 0; k < extracted.size(); ++k)
	{
		if (k > 0)
			joined += '\n';

		joined += QString::fromStdString(extracted[k]);
	}

	return joined;
}

void editor_view_t::load_script_entry(const std::string & old_text, const std::string & new_text)
{
	script_template_t tmpl;
	tmpl.full_line = old_text;

	size_t pos = 0;
	while (pos < old_text.size())
	{
		auto i = old_text.find('"', pos);
		if (i == std::string::npos)
			break;

		auto j = old_text.find('"', i + 1);
		if (j == std::string::npos)
			break;

		tmpl.quote_starts.push_back(i);
		tmpl.quote_ends.push_back(j);
		pos = j + 1;
	}

	if (tmpl.quote_starts.empty())
	{
		m_script_template = std::nullopt;
		m_translation_editor->setPlainText(QString::fromStdString(new_text));
		return;
	}

	m_script_template = tmpl;

	const auto original_extracted = extract_quoted_strings(old_text);
	m_original_view->setPlainText(join_extracted_lines(original_extracted));

	const auto translation_extracted = extract_quoted_strings(new_text);
	m_translation_editor->setPlainText(join_extracted_lines(translation_extracted));
}

std::string editor_view_t::reconstruct_script_line() const
{
	if (!m_script_template)
		return m_translation_editor->toPlainText().toStdString();

	const auto & tmpl = *m_script_template;
	const auto lines = m_translation_editor->toPlainText().split('\n');
	const size_t slot_count = tmpl.quote_starts.size();

	std::string result = tmpl.full_line;

	for (size_t k = slot_count; k > 0; --k)
	{
		const size_t idx = k - 1;
		std::string replacement;
		if (idx < static_cast<size_t>(lines.size()))
			replacement = lines[static_cast<int>(idx)].toStdString();

		size_t start = tmpl.quote_starts[idx] + 1;
		size_t end = tmpl.quote_ends[idx];
		result.replace(start, end - start, replacement);
	}

	return result;
}

bool editor_view_t::has_script_template() const
{
	return m_script_template.has_value();
}

size_t editor_view_t::script_slot_count() const
{
	if (!m_script_template)
		return 0;

	return m_script_template->quote_starts.size();
}

void editor_view_t::clear_script_template()
{
	m_script_template = std::nullopt;
}
