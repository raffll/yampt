#include "view/preview_view.hpp"
#include "model/editable_column_set.hpp"
#include "model/view_tree_model.hpp"
#include <decoder/field_validator.hpp>
#include <decoder/scvr_condition.hpp>
#include <scanner/record_conflict.hpp>
#include <algorithm>
#include <string>
#include <vector>
#include <QAbstractItemView>
#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollBar>
#include <QStringList>
#include <QTextCharFormat>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

constexpr int controls_margin = 2;
constexpr int control_character_width = 24;

void set_plain_text_reset(QTextEdit * edit, const std::string & text)
{
	edit->clear();
	edit->setCurrentCharFormat(QTextCharFormat {});
	edit->setPlainText(QString::fromStdString(text));
}

std::vector<std::string> split_lines(const std::string & text)
{
	std::vector<std::string> lines;
	std::string current;

	for (const char character : text)
	{
		if (character != '\n')
		{
			current += character;
			continue;
		}

		if (!current.empty() && current.back() == '\r')
			current.pop_back();

		lines.push_back(current);
		current.clear();
	}

	if (!current.empty() && current.back() == '\r')
		current.pop_back();

	lines.push_back(current);

	return lines;
}

std::string trim_indentation(const std::string & line)
{
	const auto start = line.find_first_not_of(" \t\r");
	if (start == std::string::npos)
		return {};

	return line.substr(start);
}

QString line_to_html(const std::string & line, const char * background)
{
	const auto escaped = QString::fromStdString(line).toHtmlEscaped();
	if (background == nullptr)
		return escaped;

	return QString("<span style='background-color:%1;'>").arg(background) + escaped + "</span>";
}

std::vector<std::vector<int>> build_line_lcs(
    const std::vector<std::string> & left_keys,
    const std::vector<std::string> & right_keys)
{
	const auto rows = left_keys.size();
	const auto cols = right_keys.size();
	std::vector<std::vector<int>> matrix(rows + 1, std::vector<int>(cols + 1, 0));

	for (size_t row = 1; row <= rows; ++row)
	{
		for (size_t col = 1; col <= cols; ++col)
		{
			if (left_keys[row - 1] == right_keys[col - 1])
				matrix[row][col] = matrix[row - 1][col - 1] + 1;
			else
				matrix[row][col] = std::max(matrix[row - 1][col], matrix[row][col - 1]);
		}
	}

	return matrix;
}

void build_line_diff_html(
    const std::string & left_text,
    const std::string & right_text,
    QString & left_html,
    QString & right_html)
{
	const auto left_lines = split_lines(left_text);
	const auto right_lines = split_lines(right_text);

	std::vector<std::string> left_keys;
	std::vector<std::string> right_keys;
	left_keys.reserve(left_lines.size());
	right_keys.reserve(right_lines.size());

	for (const auto & line : left_lines)
		left_keys.push_back(trim_indentation(line));

	for (const auto & line : right_lines)
		right_keys.push_back(trim_indentation(line));

	const auto matrix = build_line_lcs(left_keys, right_keys);

	auto row = left_lines.size();
	auto col = right_lines.size();
	QStringList left_ordered;
	QStringList right_ordered;

	while (row > 0 || col > 0)
	{
		if (row > 0 && col > 0 && left_keys[row - 1] == right_keys[col - 1])
		{
			left_ordered.push_front(line_to_html(left_lines[row - 1], nullptr));
			right_ordered.push_front(line_to_html(right_lines[col - 1], nullptr));
			--row;
			--col;

			continue;
		}

		if (col > 0 && (row == 0 || matrix[row][col - 1] >= matrix[row - 1][col]))
		{
			right_ordered.push_front(line_to_html(right_lines[col - 1], "#ccffcc"));
			--col;

			continue;
		}

		left_ordered.push_front(line_to_html(left_lines[row - 1], "#ffcccc"));
		--row;
	}

	const auto wrap = [](const QStringList & lines)
	{
		return QString("<div style='white-space:pre-wrap;'>") + lines.join(QString("\n")) + "</div>";
	};

	left_html = wrap(left_ordered);
	right_html = wrap(right_ordered);
}

} // namespace

preview_view_t::preview_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * outer_layout = new QVBoxLayout(this);
	outer_layout->setContentsMargins(0, 0, 0, 0);
	outer_layout->setSpacing(controls_margin);

	auto * comparison_layout = new QHBoxLayout();
	comparison_layout->setContentsMargins(0, 0, 0, 0);
	comparison_layout->setSpacing(4);

	const qreal tab_stop_distance = fontMetrics().horizontalAdvance(QChar(' ')) * 4;

	m_left_edit = new QTextEdit(this);
	m_left_edit->setReadOnly(true);
	m_left_edit->setPlaceholderText(tr("Previous plugin"));
	m_left_edit->setTabStopDistance(tab_stop_distance);
	comparison_layout->addWidget(m_left_edit);

	m_right_edit = new QTextEdit(this);
	m_right_edit->setReadOnly(true);
	m_right_edit->setPlaceholderText(tr("Selected plugin"));
	m_right_edit->setTabStopDistance(tab_stop_distance);
	m_right_edit->installEventFilter(this);
	comparison_layout->addWidget(m_right_edit);

	outer_layout->addLayout(comparison_layout);

	m_controls_widget = new QWidget(this);

	auto * controls_layout = new QHBoxLayout(m_controls_widget);
	controls_layout->setContentsMargins(0, 0, controls_margin, controls_margin);
	controls_layout->setSpacing(4);

	const int control_width = fontMetrics().averageCharWidth() * control_character_width;
	const int diff_toggle_width = fontMetrics().averageCharWidth() * 8;

	m_diff_toggle_button = new QPushButton(tr("Diff"), m_controls_widget);
	m_diff_toggle_button->setToolTip(tr("Toggle highlighting of differences between the two panels"));
	m_diff_toggle_button->setCheckable(true);
	m_diff_toggle_button->setChecked(true);
	m_diff_toggle_button->setFixedWidth(diff_toggle_width);
	controls_layout->addWidget(m_diff_toggle_button);

	controls_layout->addStretch(1);

	m_value_selector = new QComboBox(m_controls_widget);
	m_value_selector->setVisible(false);
	m_value_selector->setToolTip(tr("Select a value from the list"));
	m_value_selector->setFixedWidth(control_width);
	m_value_selector->view()->installEventFilter(this);
	controls_layout->addWidget(m_value_selector);

	m_apply_button = new QPushButton(tr("Apply"), m_controls_widget);
	m_apply_button->setToolTip(tr("Apply field edit to the loaded plugin"));
	m_apply_button->setEnabled(false);
	m_apply_button->setFixedWidth(control_width);
	controls_layout->addWidget(m_apply_button);

	outer_layout->addWidget(m_controls_widget);

	connect(m_right_edit, &QTextEdit::textChanged, this, &preview_view_t::on_text_changed);
	connect(m_apply_button, &QPushButton::clicked, this, &preview_view_t::on_apply_clicked);
	connect(m_value_selector, &QComboBox::currentTextChanged, this, &preview_view_t::on_value_selector_changed);
	connect(m_diff_toggle_button, &QPushButton::toggled, this, &preview_view_t::on_diff_toggled);

	setup_scroll_sync();
}

void preview_view_t::setup_scroll_sync()
{
	connect(
	    m_left_edit->verticalScrollBar(),
	    &QScrollBar::valueChanged,
	    this,
	    [this]() { sync_scroll_from(m_left_edit); });
	connect(
	    m_left_edit->horizontalScrollBar(),
	    &QScrollBar::valueChanged,
	    this,
	    [this]() { sync_scroll_from(m_left_edit); });
	connect(
	    m_right_edit->verticalScrollBar(),
	    &QScrollBar::valueChanged,
	    this,
	    [this]() { sync_scroll_from(m_right_edit); });
	connect(
	    m_right_edit->horizontalScrollBar(),
	    &QScrollBar::valueChanged,
	    this,
	    [this]() { sync_scroll_from(m_right_edit); });
}

void preview_view_t::sync_scroll_from(QTextEdit * source_edit)
{
	if (m_scroll_syncing || !m_scroll_sync_enabled)
		return;

	m_scroll_syncing = true;

	auto * target_edit = (source_edit == m_left_edit) ? m_right_edit : m_left_edit;
	auto * source_vertical = source_edit->verticalScrollBar();
	auto * source_horizontal = source_edit->horizontalScrollBar();
	auto * target_vertical = target_edit->verticalScrollBar();

	if (source_vertical->maximum() > 0)
	{
		const double ratio = static_cast<double>(source_vertical->value()) / source_vertical->maximum();
		target_vertical->setValue(static_cast<int>(ratio * target_vertical->maximum()));
	}

	target_edit->horizontalScrollBar()->setValue(source_horizontal->value());

	m_scroll_syncing = false;
}

void preview_view_t::set_scroll_sync(bool enabled)
{
	m_scroll_sync_enabled = enabled;
}

bool preview_view_t::eventFilter(QObject * watched, QEvent * event)
{
	if (watched == m_right_edit && event->type() == QEvent::KeyPress)
		m_user_has_typed = true;

	if (watched == m_value_selector->view() && event->type() == QEvent::MouseButtonRelease)
	{
		auto * mouse_event = static_cast<QMouseEvent *>(event);
		const auto view_index = m_value_selector->view()->indexAt(mouse_event->pos());
		if (view_index.isValid())
		{
			auto * item_model = m_value_selector->model();
			const auto current_state = item_model->data(view_index, Qt::CheckStateRole).toInt();
			const auto new_state = (current_state == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
			item_model->setData(view_index, new_state, Qt::CheckStateRole);
			on_value_selector_changed();
			return true;
		}
	}

	return QWidget::eventFilter(watched, event);
}

void preview_view_t::show_comparison(const std::string & left_raw, const std::string & right_raw)
{
	m_left_cached = (left_raw == non_existent_value) ? std::string {} : left_raw;
	m_right_cached = (right_raw == non_existent_value) ? std::string {} : right_raw;

	render_comparison();
}

void preview_view_t::render_comparison()
{
	const auto & left_text = m_left_cached;
	const auto & right_text = m_right_cached;

	if (left_text.empty())
	{
		m_left_edit->clear();
		m_right_edit->setPlainText(QString::fromStdString(right_text));
		return;
	}

	if (right_text.empty())
	{
		m_left_edit->setPlainText(QString::fromStdString(left_text));
		m_right_edit->clear();
		return;
	}

	if (!m_diff_coloring_enabled || left_text == right_text)
	{
		set_plain_text_reset(m_left_edit, left_text);
		set_plain_text_reset(m_right_edit, right_text);
		return;
	}

	QString left_html;
	QString right_html;
	build_line_diff_html(left_text, right_text, left_html, right_html);

	m_left_edit->setHtml(left_html);
	m_right_edit->setHtml(right_html);
}

void preview_view_t::on_diff_toggled(bool enabled)
{
	m_diff_coloring_enabled = enabled;

	const int left_scroll = m_left_edit->verticalScrollBar()->value();
	const int right_scroll = m_right_edit->verticalScrollBar()->value();
	const int left_scroll_h = m_left_edit->horizontalScrollBar()->value();
	const int right_scroll_h = m_right_edit->horizontalScrollBar()->value();

	render_comparison();

	m_left_edit->verticalScrollBar()->setValue(left_scroll);
	m_right_edit->verticalScrollBar()->setValue(right_scroll);
	m_left_edit->horizontalScrollBar()->setValue(left_scroll_h);
	m_right_edit->horizontalScrollBar()->setValue(right_scroll_h);
}

void preview_view_t::clear()
{
	m_left_cached.clear();
	m_right_cached.clear();
	m_left_edit->clear();
	m_right_edit->clear();
	set_editing_enabled(false);
	m_value_selector->setVisible(false);
}

void preview_view_t::set_editing_enabled(bool enabled)
{
	m_right_edit->setReadOnly(!enabled);
	m_editing_active = enabled;
	m_user_has_typed = false;
	emit validation_message({});

	if (!enabled)
	{
		m_right_edit->setStyleSheet("");
		m_apply_button->setEnabled(false);
		m_value_selector->setVisible(false);
	}
}

void preview_view_t::set_edit_controller(field_edit_controller_t * controller)
{
	m_edit_controller = controller;
}

void preview_view_t::set_editable_columns(const editable_column_set_t * columns)
{
	m_editable_columns = columns;
}

void preview_view_t::update_selection(
    const QModelIndex & index,
    const view_tree_model_t * model,
    const std::string & cell_value)
{
	if (!index.isValid() || !m_editable_columns)
	{
		set_editing_enabled(false);
		return;
	}

	const int column = index.column();
	if (column < 1)
	{
		set_editing_enabled(false);
		return;
	}

	if (!m_editable_columns->is_editable(column))
	{
		set_editing_enabled(false);
		return;
	}

	const auto label_index = index.siblingAtColumn(0);
	const auto label = model->data(label_index, Qt::DisplayRole).toString();
	if (label == "Signature" || label == "Record Flags")
	{
		set_editing_enabled(false);
		return;
	}

	if (cell_value == non_existent_value)
	{
		set_editing_enabled(false);
		return;
	}

	const int plugin_idx =
	    model->is_merge_column(column) ? -1 : model->column_plugin_indices()[static_cast<size_t>(column) - 1];

	m_pending_request.record_type = model->record_type();
	m_pending_request.record_id = model->record_id();
	m_pending_request.codepage = model->display_codepage();
	m_pending_request.plugin_idx = plugin_idx;
	m_pending_request.record_index = model->record_index_for_column(column);
	m_pending_request.field = {};

	const auto field_variant = model->data(index, view_tree_model_t::field_def_role);
	if (field_variant.isValid() && !field_variant.isNull())
	{
		const auto * field_ptr = field_variant.value<const field_def_t *>();
		if (field_ptr)
			m_pending_request.field = *field_ptr;
	}

	if (m_pending_request.field.name != nullptr)
	{
		m_existing_sub_size = m_pending_request.field.size;
		if (m_existing_sub_size == 0 && m_pending_request.field.type == field_type_t::raw)
		{
			auto * parent_ptr = static_cast<const view_tree_model_t::view_node_t *>(index.internalPointer());
			if (parent_ptr)
				m_existing_sub_size = parent_ptr->size;
		}
	}
	else
	{
		set_editing_enabled(false);
		return;
	}

	const auto occurrence_variant = model->data(index, view_tree_model_t::sub_record_occurrence_role);
	if (occurrence_variant.isValid())
	{
		const auto occurrence = occurrence_variant.value<view_tree_model_t::sub_record_occurrence_t>();
		m_pending_request.sub_type = occurrence.sub_type;
		m_pending_request.occurrence = occurrence.occurrence;
		m_pending_request.object_ref_index = occurrence.object_ref_index;
	}

	m_original_value = cell_value;
	populate_value_selector();
	set_editing_enabled(true);
	m_right_edit->setPlainText(QString::fromStdString(cell_value));
}

void preview_view_t::on_text_changed()
{
	if (!m_user_has_typed)
		return;

	if (!m_editing_active)
		return;

	const auto current_text = m_right_edit->toPlainText().toStdString();

	bool is_valid = true;
	std::string error_message;
	if (m_pending_request.field.name != nullptr)
	{
		const auto result = field_validator::validate_field(
		    m_pending_request.field, current_text, m_pending_request.codepage, m_existing_sub_size);

		is_valid = result.valid;
		error_message = result.error_message;
	}

	if (!is_valid)
	{
		m_right_edit->setStyleSheet("background-color: #ffcccc;");
		emit validation_message(QString::fromStdString(error_message));
	}
	else
	{
		m_right_edit->setStyleSheet("");
		emit validation_message({});
	}

	const bool value_changed = (current_text != m_original_value);
	m_apply_button->setEnabled(is_valid && value_changed);
}

void preview_view_t::on_apply_clicked()
{
	if (!m_edit_controller)
		return;

	m_pending_request.input_text = m_right_edit->toPlainText().toStdString();

	const auto result = m_edit_controller->commit_field_edit(m_pending_request);

	if (result.success)
	{
		emit edit_committed();
		return;
	}

	emit validation_message(QString::fromStdString(result.error_message));
}

void preview_view_t::on_value_selector_changed()
{
	if (!m_editing_active)
		return;

	const auto & field = m_pending_request.field;
	const bool is_flags =
	    (field.type == field_type_t::flags_u8 || field.type == field_type_t::flags_u16 ||
	     field.type == field_type_t::flags_u32);

	QString new_text;

	if (is_flags)
	{
		QStringList checked_names;
		auto * item_model = m_value_selector->model();

		for (int row = 0; row < m_value_selector->count(); ++row)
		{
			auto item_index = item_model->index(row, 0);
			const auto check_state = item_model->data(item_index, Qt::CheckStateRole).toInt();

			if (check_state == Qt::Checked)
				checked_names.append(m_value_selector->itemText(row));
		}

		new_text = checked_names.join(" | ");
	}
	else
	{
		new_text = m_value_selector->currentText();
	}

	m_user_has_typed = true;
	m_right_edit->setPlainText(new_text);
}

void preview_view_t::populate_value_selector()
{
	m_value_selector->clear();
	m_value_selector->setVisible(false);

	if (m_pending_request.field.name == nullptr)
		return;

	const auto & field = m_pending_request.field;

	switch (field.type)
	{
	case field_type_t::enum_u8:
	case field_type_t::enum_u16:
	case field_type_t::enum_u32:
	{
		if (!field.enum_names)
			return;

		for (const char * const * current = field.enum_names; *current != nullptr; ++current)
			m_value_selector->addItem(QString::fromUtf8(*current));

		m_value_selector->setCurrentText(QString::fromStdString(m_original_value));
		m_value_selector->setVisible(true);
		break;
	}

	case field_type_t::i8:
	case field_type_t::i32:
	{
		if (!field.enum_names)
			return;

		m_value_selector->addItem(tr("None"));
		for (const char * const * current = field.enum_names; *current != nullptr; ++current)
			m_value_selector->addItem(QString::fromUtf8(*current));

		m_value_selector->setCurrentText(QString::fromStdString(m_original_value));
		m_value_selector->setVisible(true);
		break;
	}

	case field_type_t::bool_bit:
	{
		m_value_selector->addItem(tr("Yes"));
		m_value_selector->addItem(tr("No"));
		m_value_selector->setCurrentText(QString::fromStdString(m_original_value));
		m_value_selector->setVisible(true);
		break;
	}

	case field_type_t::flags_u8:
	case field_type_t::flags_u16:
	case field_type_t::flags_u32:
	{
		populate_flags_selector(field);
		break;
	}

	case field_type_t::scvr_type:
	{
		for (const auto & type_name : scvr_type_names())
			m_value_selector->addItem(QString::fromStdString(type_name));

		m_value_selector->setCurrentText(QString::fromStdString(m_original_value));
		m_value_selector->setVisible(true);
		break;
	}

	case field_type_t::scvr_operator:
	{
		for (const auto & operator_symbol : scvr_operator_symbols())
			m_value_selector->addItem(QString::fromStdString(operator_symbol));

		m_value_selector->setCurrentText(QString::fromStdString(m_original_value));
		m_value_selector->setVisible(true);
		break;
	}

	default:
		break;
	}
}

void preview_view_t::populate_flags_selector(const field_def_t & field)
{
	if (!field.flag_names)
		return;

	const auto current_flags = QString::fromStdString(m_original_value);

	for (int bit_pos = 0; bit_pos < field.flag_count; ++bit_pos)
	{
		if (field.flag_names[bit_pos][0] == '_')
			continue;

		const auto flag_name = QString::fromUtf8(field.flag_names[bit_pos]);
		m_value_selector->addItem(flag_name);

		auto * item_model = m_value_selector->model();
		const int row = m_value_selector->count() - 1;
		auto item_index = item_model->index(row, 0);

		const bool is_set = current_flags.contains(flag_name);
		item_model->setData(item_index, is_set ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);
	}

	m_value_selector->setVisible(true);
}
