#pragma once

#include <utility/status_types.hpp>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <QWidget>

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
	void refresh_theme();

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

	QHBoxLayout * m_layout = nullptr;
	std::vector<status_button_t> m_status_buttons;
	std::map<status_t, size_t> m_current_counts;
	std::set<status_t> m_active_statuses;
	std::set<status_t> m_saved_statuses;
	bool m_solo = false;
	status_t m_solo_status = status_t::untranslated;
	bool m_document_open = false;
};
