#pragma once

#include <QWidget>

class QTableWidget;
class QPushButton;
class settings_store_t;

class sub_record_rules_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit sub_record_rules_view_t(QWidget * parent = nullptr);

	void load(const settings_store_t & settings);
	void save(settings_store_t & settings) const;

private:
	void add_row(const QString & record, const QString & sub_record, bool ignore, bool exclude, bool skip);
	void remove_selected_row();

	QTableWidget * m_table = nullptr;
	QPushButton * m_add_button = nullptr;
	QPushButton * m_remove_button = nullptr;
};
