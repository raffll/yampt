#include "view/preview_view.hpp"
#include "model/editable_column_set.hpp"
#include "model/view_tree_model.hpp"
#include <decoder/field_validator.hpp>
#include <decoder/scvr_condition.hpp>
#include <scanner/record_conflict.hpp>
#include <utility/char_diff.hpp>
#include <utility/record_behavior.hpp>
#include <string>
#include <vector>
#include <QAbstractItemView>
#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollBar>
#include <QStringList>
#include <QTextCharFormat>
#include <QTextEdit>
#include <QVBoxLayout>
#include <theme_system.hpp>

namespace {

constexpr int controls_margin = 2;
constexpr int control_character_width = 24;

void set_plain_text_reset(QTextEdit * edit, const std::string & text)
{
	edit->clear();
	edit->setCurrentCharFormat(QTextCharFormat {});
	edit->setPlainText(QString::fromStdString(text));
}

QList<QTextEdit::ExtraSelection> build_diff_selections(
    QTextEdit * edit,
    const std::vector<diff_segment_t> & segments,
    diff_op_t pane_operation,
    color_name_t color)
{
	QList<QTextEdit::ExtraSelection> selections;

	QTextCharFormat diff_format;
	diff_format.setBackground(theme_system_t::instance().get_color(color));

	int char_position = 0;
	for (const auto & segment : segments)
	{
		if (segment.operation != diff_op_t::unchanged && segment.operation != pane_operation)
			continue;

		const int char_length = static_cast<int>(QString::fromStdString(segment.text).size());

		if (segment.operation == pane_operation)
		{
			QTextEdit::ExtraSelection selection;
			selection.format = diff_format;
			selection.cursor = edit->textCursor();
			selection.cursor.setPosition(char_position);
			selection.cursor.setPosition(char_position + char_length, QTextCursor::KeepAnchor);
			selections.append(selection);
		}

		char_position += char_length;
	}

	return selections;
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
	controls_layout->setContentsMargins(controls_margin, 0, controls_margin, controls_margin);
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

	m_message_label = new QLabel(m_controls_widget);
	m_message_label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	controls_layout->addWidget(m_message_label);

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
	    m_left_edit->verticalScrollBar(), &QScrollBar::valueChanged, this, [this]() { sync_scroll_from(m_left_edit); });
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
	set_plain_text_reset(m_left_edit, m_left_cached);
	set_plain_text_reset(m_right_edit, m_right_cached);

	apply_diff_highlighting();
}

void preview_view_t::apply_diff_highlighting()
{
	m_left_edit->setExtraSelections({});
	m_right_edit->setExtraSelections({});

	const auto left_text = QString::fromStdString(m_left_cached).toStdString();
	const auto right_text = QString::fromStdString(m_right_cached).toStdString();

	if (!m_diff_coloring_enabled || left_text.empty() || right_text.empty() || left_text == right_text)
		return;

	const auto segments = compute_char_diff(left_text, right_text);

	m_left_edit->setExtraSelections(
	    build_diff_selections(m_left_edit, segments, diff_op_t::deleted, color_name_t::diff_removed_background));
	m_right_edit->setExtraSelections(
	    build_diff_selections(m_right_edit, segments, diff_op_t::inserted, color_name_t::diff_added_background));
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
	m_left_edit->setExtraSelections({});
	m_right_edit->setExtraSelections({});
	set_editing_enabled(false);
	m_value_selector->setVisible(false);
}

void preview_view_t::set_editing_enabled(bool enabled)
{
	m_right_edit->setReadOnly(!enabled);
	m_editing_active = enabled;
	m_user_has_typed = false;

	if (!enabled)
	{
		m_right_edit->setStyleSheet("");
		m_apply_button->setEnabled(false);
		m_value_selector->setVisible(false);
		m_message_label->clear();
	}
}

void preview_view_t::show_error_message(const QString & message)
{
	m_message_label->setStyleSheet("color: rgb(220, 50, 50);");
	m_message_label->setText(tr("Error: %1").arg(message));
}

void preview_view_t::show_range_hint()
{
	const auto & field = m_pending_request.field;
	if (field.name == nullptr)
	{
		m_message_label->clear();
		return;
	}

	const auto hint = field_validator::range_hint(field, m_existing_sub_size);
	if (hint.empty())
	{
		m_message_label->clear();
		return;
	}

	m_message_label->setStyleSheet("");
	m_message_label->setText(tr("Range: %1").arg(QString::fromStdString(hint)));
}

void preview_view_t::show_readonly_message(const QString & message)
{
	m_message_label->setStyleSheet("");
	m_message_label->setText(message);
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

	m_pending_request.record_type = model->record_type();
	m_pending_request.record_id = model->record_id();
	m_pending_request.codepage = model->display_codepage();
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

	const bool is_leveled = decode_mode_for(m_pending_request.record_type) == decode_mode_t::leveled;
	if (is_leveled && m_pending_request.sub_type == "INDX")
	{
		set_editing_enabled(false);
		m_right_cached = (cell_value == non_existent_value) ? std::string {} : cell_value;
		render_comparison();
		show_readonly_message(tr("Auto-calculated, not editable"));
		return;
	}

	const char * const record_id_sub_type = record_id_sub_type_for(m_pending_request.record_type);
	if (record_id_sub_type != nullptr && m_pending_request.sub_type == record_id_sub_type)
	{
		set_editing_enabled(false);
		m_right_cached = (cell_value == non_existent_value) ? std::string {} : cell_value;
		render_comparison();
		show_readonly_message(tr("Record ID, not editable"));
		return;
	}

	if (read_only_reason_for(m_pending_request.record_type) == read_only_reason_t::landscape_data)
	{
		set_editing_enabled(false);
		m_right_cached = (cell_value == non_existent_value) ? std::string {} : cell_value;
		render_comparison();
		show_readonly_message(tr("Landscape data, not editable"));
		return;
	}

	m_original_value = cell_value;
	populate_value_selector();
	set_editing_enabled(true);
	show_range_hint();

	m_right_cached = (cell_value == non_existent_value) ? std::string {} : cell_value;
	render_comparison();
}

void preview_view_t::on_text_changed()
{
	if (!m_user_has_typed)
		return;

	if (!m_editing_active)
		return;

	const auto current_text = m_right_edit->toPlainText().toStdString();

	m_right_cached = current_text;
	apply_diff_highlighting();

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
		show_error_message(QString::fromStdString(error_message));
	}
	else
	{
		m_right_edit->setStyleSheet("");
		show_range_hint();
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

	show_error_message(QString::fromStdString(result.error_message));
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

	case field_type_t::global_type:
	{
		m_value_selector->addItem(tr("Short"));
		m_value_selector->addItem(tr("Long"));
		m_value_selector->addItem(tr("Float"));
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
