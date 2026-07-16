#pragma once

#include <utility/status_types.hpp>
#include <map>
#include <set>
#include <vector>
#include <QWidget>

class QListWidget;
class QListWidgetItem;

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
	void set_visible_statuses(const std::set<status_t> & visible);
	void refresh_theme();

signals:
	void filters_changed();

private:
	void on_item_clicked(QListWidgetItem * item);
	void on_item_right_clicked(QListWidgetItem * item);
	void build_rows();
	void update_all_state();
	void update_styles();

	struct row_t
	{
		QListWidgetItem * item = nullptr;
		status_t status = status_t::untranslated;
		bool is_all = false;
		bool selected = true;
	};

	QListWidget * m_list = nullptr;
	std::vector<row_t> m_rows;
	int m_all_row = -1;
	bool m_enabled = true;
	bool m_document_open = false;
	std::set<status_t> m_visible_statuses;
};
