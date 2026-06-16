#pragma once

#include <QWidget>
#include <map>
#include <set>
#include <string>
#include <vector>

class QHBoxLayout;
class QPushButton;

class status_filter_bar_t : public QWidget
{
	Q_OBJECT

public:
	explicit status_filter_bar_t(QWidget * parent = nullptr);

	void update_counts(const std::map<std::string, size_t> & counts);
	std::set<std::string> get_active_statuses() const;
	bool has_filter() const;
	void set_filter_state(const std::set<std::string> & statuses);

signals:
	void filters_changed();

private:
	void on_status_clicked(const std::string & status);
	void on_status_right_clicked(const std::string & status);
	void update_button_styles();

	struct status_button_t
	{
		QPushButton * button = nullptr;
		std::string status;
		size_t count = 0;
	};

	QHBoxLayout * layout_ = nullptr;
	std::vector<status_button_t> status_buttons_;
	std::map<std::string, size_t> current_counts_;
	std::set<std::string> active_statuses_;
	std::set<std::string> saved_statuses_;
	bool solo_ = false;
	std::string solo_status_;
};
