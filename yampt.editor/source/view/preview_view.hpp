#pragma once

#include "controller/field_edit_controller.hpp"
#include <string>
#include <QWidget>

class QComboBox;
class QLabel;
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

	void show_comparison(const std::string & left_raw, const std::string & right_raw);
	void clear();

	void set_edit_controller(field_edit_controller_t * controller);
	void set_editable_columns(const editable_column_set_t * columns);
	void set_editing_enabled(bool enabled);
	void update_selection(const QModelIndex & index, const view_tree_model_t * model, const std::string & cell_value);

	void set_scroll_sync(bool enabled);

protected:
	bool eventFilter(QObject * watched, QEvent * event) override;

signals:
	void edit_committed();
	void validation_message(const QString & message);

private slots:
	void on_text_changed();
	void on_apply_clicked();
	void on_value_selector_changed();
	void on_diff_toggled(bool enabled);

private:
	void populate_value_selector();
	void populate_flags_selector(const field_def_t & field);
	void setup_scroll_sync();
	void sync_scroll_from(QTextEdit * source_edit);
	void render_comparison();

	QTextEdit * m_left_edit = nullptr;
	QTextEdit * m_right_edit = nullptr;
	QWidget * m_controls_widget = nullptr;
	QComboBox * m_value_selector = nullptr;
	QPushButton * m_apply_button = nullptr;
	QPushButton * m_diff_toggle_button = nullptr;

	field_edit_controller_t * m_edit_controller = nullptr;
	const editable_column_set_t * m_editable_columns = nullptr;

	field_edit_request_t m_pending_request;
	bool m_editing_active = false;
	bool m_user_has_typed = false;
	std::string m_original_value;
	size_t m_existing_sub_size = 0;

	std::string m_left_cached;
	std::string m_right_cached;
	bool m_diff_coloring_enabled = true;

	bool m_scroll_syncing = false;
	bool m_scroll_sync_enabled = true;
};
