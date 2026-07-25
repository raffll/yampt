#pragma once

#include <QWidget>

class QLineEdit;
class settings_store_t;

class sub_record_rules_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit sub_record_rules_view_t(QWidget * parent = nullptr);

	void load(const settings_store_t & settings);
	void save(settings_store_t & settings) const;

private:
	QLineEdit * m_ignore_conflict_edit = nullptr;
	QLineEdit * m_exclude_from_merge_edit = nullptr;
	QLineEdit * m_skip_if_missing_edit = nullptr;
};
