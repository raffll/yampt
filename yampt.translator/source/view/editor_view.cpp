#include "editor_view.hpp"
#include "line_number_gutter.hpp"
#include <utility/char_diff.hpp>
#include <algorithm>
#include <theme_system.hpp>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QScrollBar>
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

	m_original_label = new QLabel(tr("Original"), left_widget);
	m_original_label->setAlignment(Qt::AlignCenter);

	m_original_view = new translation_edit_view_t(left_widget);
	m_original_view->setReadOnly(true);

	auto * original_container = new QWidget(left_widget);
	auto * original_hlayout = new QHBoxLayout(original_container);
	original_hlayout->setContentsMargins(0, 0, 0, 0);
	original_hlayout->setSpacing(0);
	original_hlayout->addWidget(new line_number_gutter_t(m_original_view, original_container));
	original_hlayout->addWidget(m_original_view);

	m_adapted_from_view = new translation_edit_view_t(left_widget);
	m_adapted_from_view->setReadOnly(true);

	m_adapted_from_container = new QWidget(left_widget);
	auto * adapted_hlayout = new QHBoxLayout(m_adapted_from_container);
	adapted_hlayout->setContentsMargins(0, 0, 0, 0);
	adapted_hlayout->setSpacing(0);
	adapted_hlayout->addWidget(new line_number_gutter_t(m_adapted_from_view, m_adapted_from_container));
	adapted_hlayout->addWidget(m_adapted_from_view);
	m_adapted_from_container->setVisible(false);

	m_adapted_toggle = new QPushButton(tr("Details"), left_widget);
	m_adapted_toggle->setCheckable(true);
	m_adapted_toggle->setChecked(true);
	m_adapted_toggle->setToolTip(tr("Show/hide adapted from panel"));
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

	m_translation_label = new QLabel(tr("Translation"), right_widget);
	m_translation_label->setAlignment(Qt::AlignCenter);

	m_translation_editor = new translation_edit_view_t(right_widget);

	m_apply_button = new QPushButton(tr("Next (Shift+Enter)"), right_widget);
	m_apply_button->setToolTip(tr("Apply changes and move to next entry"));

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

	auto sync_from = [this](QAbstractScrollArea * source_widget)
	{
		if (m_scroll_syncing || !m_scroll_sync_enabled)
			return;

		m_scroll_syncing = true;

		QAbstractScrollArea * others[] = { m_original_view, m_translation_editor, m_adapted_from_view };

		auto * source_v = source_widget->verticalScrollBar();
		auto * source_h = source_widget->horizontalScrollBar();

		for (auto * other : others)
		{
			if (other == source_widget)
				continue;

			if (source_v->maximum() > 0)
			{
				double ratio = static_cast<double>(source_v->value()) / source_v->maximum();
				other->verticalScrollBar()->setValue(static_cast<int>(ratio * other->verticalScrollBar()->maximum()));
			}

			other->horizontalScrollBar()->setValue(source_h->value());
		}

		m_scroll_syncing = false;
	};

	connect(m_original_view->verticalScrollBar(), &QScrollBar::valueChanged, this, [this, sync_from]() { sync_from(m_original_view); });
	connect(m_original_view->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this, sync_from]() { sync_from(m_original_view); });
	connect(m_translation_editor->verticalScrollBar(), &QScrollBar::valueChanged, this, [this, sync_from]() { sync_from(m_translation_editor); });
	connect(m_translation_editor->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this, sync_from]() { sync_from(m_translation_editor); });
	connect(m_adapted_from_view->verticalScrollBar(), &QScrollBar::valueChanged, this, [this, sync_from]() { sync_from(m_adapted_from_view); });
	connect(m_adapted_from_view->horizontalScrollBar(), &QScrollBar::valueChanged, this, [this, sync_from]() { sync_from(m_adapted_from_view); });
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
    const std::string & adapted_from)
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

	std::string display_text = first_segment;
	if (!remainder.empty())
	{
		auto remainder_display = remainder;
		for (auto & ch : remainder_display)
		{
			if (ch == '|')
				ch = '\n';
		}
		display_text += remainder_display;
	}
	m_adapted_from_view->setPlainText(QString::fromStdString(display_text));

	const auto q_display = QString::fromStdString(first_segment);
	const auto q_new_text = QString::fromStdString(new_text);
	const auto segments = compute_char_diff(q_new_text.toStdString(), q_display.toStdString());

	QTextCharFormat diff_format;
	diff_format.setBackground(theme_system_t::instance().get_color(color_name_t::diff_changed_background));

	int char_position = 0;
	for (const auto & segment : segments)
	{
		const auto q_segment = QString::fromStdString(segment.text);
		const int char_length = static_cast<int>(q_segment.size());

		if (segment.operation == diff_op_t::inserted)
		{
			QTextEdit::ExtraSelection sel;
			sel.format = diff_format;
			sel.cursor = m_adapted_from_view->textCursor();
			sel.cursor.setPosition(char_position);
			sel.cursor.setPosition(char_position + char_length, QTextCursor::KeepAnchor);
			selections.append(sel);
			char_position += char_length;
		}
		else if (segment.operation == diff_op_t::deleted)
		{
			const int total_chars = static_cast<int>(q_display.size());
			const int mark_pos = char_position < total_chars ? char_position : char_position - 1;
			if (mark_pos >= 0 && mark_pos < total_chars)
			{
				QTextEdit::ExtraSelection sel;
				sel.format = diff_format;
				sel.cursor = m_adapted_from_view->textCursor();
				sel.cursor.setPosition(mark_pos);
				sel.cursor.setPosition(mark_pos + 1, QTextCursor::KeepAnchor);
				selections.append(sel);
			}
		}
		else if (segment.operation == diff_op_t::unchanged)
		{
			char_position += char_length;
		}
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
	bool skip_first = false;

	auto lower = source_text;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });

	auto arrow_pos = lower.find("->");
	auto keyword_start = (arrow_pos != std::string::npos) ? arrow_pos + 2 : 0;

	while (keyword_start < lower.size() && (lower[keyword_start] == ' ' || lower[keyword_start] == '\t'))
		++keyword_start;

	if (lower.compare(keyword_start, 3, "say") == 0)
		skip_first = true;

	bool first = true;
	while (pos < source_text.size())
	{
		auto i = source_text.find('"', pos);
		if (i == std::string::npos)
			break;

		auto j = source_text.find('"', i + 1);
		if (j == std::string::npos)
			break;

		if (first && skip_first)
		{
			first = false;
			pos = j + 1;
			continue;
		}

		first = false;
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

	auto lower = old_text;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });

	auto arrow_pos = lower.find("->");
	auto keyword_start = (arrow_pos != std::string::npos) ? arrow_pos + 2 : size_t(0);

	while (keyword_start < lower.size() && (lower[keyword_start] == ' ' || lower[keyword_start] == '\t'))
		++keyword_start;

	bool skip_first = (lower.compare(keyword_start, 3, "say") == 0);

	size_t pos = 0;
	bool first = true;
	while (pos < old_text.size())
	{
		auto i = old_text.find('"', pos);
		if (i == std::string::npos)
			break;

		auto j = old_text.find('"', i + 1);
		if (j == std::string::npos)
			break;

		if (first && skip_first)
		{
			first = false;
			pos = j + 1;
			continue;
		}

		first = false;
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

void editor_view_t::set_scroll_sync(bool enabled)
{
	m_scroll_sync_enabled = enabled;
}
