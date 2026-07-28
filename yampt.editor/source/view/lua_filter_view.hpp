#pragma once

#include <set>
#include <string>
#include <vector>
#include <QWidget>
#include <scanner/conflict_detector.hpp>

class QLabel;
class QListWidget;
class QPushButton;

class lua_filter_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit lua_filter_view_t(QWidget * parent = nullptr);

	void set_conflicts(const std::vector<handler_conflict_t> & conflicts);
	std::set<conflict_severity_t> enabled_severities() const;
	std::set<std::string> enabled_interfaces() const;

signals:
	void filters_changed();

private:
	void setup_severity_buttons();
	void setup_interface_list();
	void setup_count_labels();
	void populate_interface_list(const std::vector<handler_conflict_t> & conflicts);
	void update_counts(const std::vector<handler_conflict_t> & conflicts);

	QPushButton * m_blocking_button = nullptr;
	QPushButton * m_mutating_button = nullptr;
	QPushButton * m_overlapping_button = nullptr;
	QListWidget * m_interface_list = nullptr;
	QLabel * m_total_label = nullptr;
	QLabel * m_severity_label = nullptr;
};
