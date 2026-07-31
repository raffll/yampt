#pragma once

#include <string>
#include <QWidget>
#include "controller/field_edit_controller.hpp"

class QComboBox;
class QTextEdit;
class QPushButton;
class QModelIndex;
class editable_column_set_t;
class view_tree_model_t;

class preview_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit preview_view_t(QWidget * parent = nullptr);

	void show_comparison(const std::string & left_text, const std::string & right_text);
	void clear();

	void set_edit_controller(field_edit_controller_t * controller);
	void set_editable_columns(const editable_column_set_t * columns);
	void update_selection(const QModelIndex & index, const view_tree_model_t * model);

signals:
	void edit_committed();

private slots:
	void on_text_changed();
	void on_apply_clicked();
	void on_value_selector_changed();

private:
	void set_editing_enabled(bool enabled);
	void populate_value_selector();

	QTextEdit * m_left_edit = nullptr;
	QTextEdit * m_right_edit = nullptr;
	QComboBox * m_value_selector = nullptr;
	QPushButton * m_apply_button = nullptr;

	field_edit_controller_t * m_edit_controller = nullptr;
	const editable_column_set_t * m_editable_columns = nullptr;

	field_edit_request_t m_pending_request;
	bool m_editing_active = false;
	std::string m_original_value;
	size_t m_existing_sub_size = 0;
};
