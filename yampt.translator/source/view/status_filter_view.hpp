#pragma once

#include <utility/status_types.hpp>
#include <QWidget>
#include <map>
#include <set>
#include <string>
#include <vector>

class QHBoxLayout;
class QPushButton;

class status_filter_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit status_filter_view_t(QWidget * parent = nullptr);

	void update_counts(
	    const std::map<status_t, size_t> & displayed_counts,
	    const std::map<status_t, size_t> & total_counts);
	std::set<status_t> get_active_statuses() const;
	bool has_filter() const;
	void set_filter_state(const std::set<status_t> & statuses);
	void set_document_open(bool open);

signals:
	void filters_changed();

private:
	void on_status_clicked(status_t status);
	void on_status_right_clicked(status_t status);
	void add_status_group(const std::vector<status_t> & statuses);
	void update_button_styles();

	struct status_button_t
	{
		QPushButton * button = nullptr;
		status_t status = status_t::untranslated;
		size_t count = 0;
	};

	QHBoxLayout * layout_ = nullptr;
	std::vector<status_button_t> status_buttons_;
	std::map<status_t, size_t> current_counts_;
	std::set<status_t> active_statuses_;
	std::set<status_t> saved_statuses_;
	bool solo_ = false;
	status_t solo_status_ = status_t::untranslated;
	bool document_open_ = false;
};
