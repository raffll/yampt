#include "view/preview_view.hpp"
#include "model/editable_column_set.hpp"
#include "model/view_tree_model.hpp"
#include <decoder/field_validator.hpp>
#include <scanner/record_conflict.hpp>
#include <utility/char_diff.hpp>
#include <QAbstractItemView>
#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QModelIndex>
#include <QMouseEvent>
#include <QPushButton>
#include <QTextEdit>
#include <QToolTip>
#include <QVBoxLayout>

preview_view_t::preview_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * outer_layout = new QHBoxLayout(this);
	outer_layout->setContentsMargins(0, 0, 0, 0);
	outer_layout->setSpacing(4);

	m_left_edit = new QTextEdit(this);
	m_left_edit->setReadOnly(true);
	m_left_edit->setPlaceholderText(tr("Previous plugin"));
	outer_layout->addWidget(m_left_edit);

	auto * right_column = new QVBoxLayout();
	right_column->setContentsMargins(0, 0, 0, 0);
	right_column->setSpacing(4);

	m_right_edit = new QTextEdit(this);
	m_right_edit->setReadOnly(true);
	m_right_edit->setPlaceholderText(tr("Selected plugin"));
	m_right_edit->installEventFilter(this);
	right_column->addWidget(m_right_edit);

	m_value_selector = new QComboBox(this);
	m_value_selector->setVisible(false);
	m_value_selector->setToolTip(tr("Select a value from the list"));
	m_value_selector->view()->installEventFilter(this);
	right_column->addWidget(m_value_selector);

	m_apply_button = new QPushButton(tr("Apply"), this);
	m_apply_button->setToolTip(tr("Commit field edit to disk"));
	m_apply_button->setVisible(false);
	right_column->addWidget(m_apply_button);

	outer_layout->addLayout(right_column);

	connect(m_right_edit, &QTextEdit::textChanged, this, &preview_view_t::on_text_changed);
	connect(m_apply_button, &QPushButton::clicked, this, &preview_view_t::on_apply_clicked);
	connect(m_value_selector, &QComboBox::currentTextChanged, this, &preview_view_t::on_value_selector_changed);
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

void preview_view_t::show_comparison(const std::string & left_text, const std::string & right_text)
{
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

	if (left_text == right_text)
	{
		m_left_edit->setPlainText(QString::fromStdString(left_text));
		m_right_edit->setPlainText(QString::fromStdString(right_text));
		return;
	}

	const auto segments = compute_char_diff(left_text, right_text);

	QString left_html;
	QString right_html;

	for (const auto & segment : segments)
	{
		auto escaped = QString::fromStdString(segment.text).toHtmlEscaped().replace("\n", "<br>");

		switch (segment.operation)
		{
		case diff_op_t::unchanged:
			left_html += escaped;
			right_html += escaped;
			break;
		case diff_op_t::deleted:
			left_html += "<span style='background-color:#ffcccc;'>" + escaped + "</span>";
			break;
		case diff_op_t::inserted:
			right_html += "<span style='background-color:#ccffcc;'>" + escaped + "</span>";
			break;
		}
	}

	m_left_edit->setHtml(left_html);
	m_right_edit->setHtml(right_html);
}

void preview_view_t::clear()
{
	m_left_edit->clear();
	m_right_edit->clear();
	set_editing_enabled(false);
	m_value_selector->setVisible(false);
}

void preview_view_t::set_editing_enabled(bool enabled)
{
	m_right_edit->setReadOnly(!enabled);
	m_apply_button->setVisible(enabled);
	m_editing_active = enabled;
	m_user_has_typed = false;

	if (!enabled)
	{
		m_right_edit->setStyleSheet("");
		m_apply_button->setEnabled(false);
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
	if (m_pending_request.field.name != nullptr)
	{
		const auto result = field_validator::validate_field(
		    m_pending_request.field, current_text, m_pending_request.codepage, m_existing_sub_size);

		is_valid = result.valid;
	}

	if (!is_valid)
		m_right_edit->setStyleSheet("background-color: #ffcccc;");
	else
		m_right_edit->setStyleSheet("");

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

	QToolTip::showText(m_apply_button->mapToGlobal(QPoint(0, 0)), QString::fromStdString(result.error_message));
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
