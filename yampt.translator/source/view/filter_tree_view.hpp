#pragma once

#include <utility/domain_types.hpp>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <QWidget>

class QListWidget;
class QListWidgetItem;

class filter_tree_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit filter_tree_view_t(QWidget * parent = nullptr);

	enum class display_mode_t
	{
		full,
		all_only,
		empty
	};
	void set_display_mode(display_mode_t mode);

	void update_counts(
	    const std::map<rec_type_t, size_t> & total_counts,
	    const std::map<rec_type_t, size_t> & translated_counts);
	void update_sub_type_counts(
	    const std::map<std::string, size_t> & sub_type_total_counts,
	    const std::map<std::string, size_t> & sub_type_translated_counts);
	void set_total_count(size_t translated, size_t total);

	std::set<rec_type_t> get_active_types() const;
	std::set<std::string> get_active_sub_types() const;
	bool has_sub_type_filter() const;
	void set_active_types(const std::set<rec_type_t> & types);
	void set_active_sub_types(const std::set<std::string> & sub_types);
	bool is_solo() const;
	rec_type_t get_solo_type() const;

	bool is_yaml_filter_active() const;
	void set_yaml_filter_active(bool active);
	void set_yaml_button_visible(bool visible);
	void setEnabled(bool enabled);

signals:
	void filters_changed();
	void all_reset_requested();

private:
	void on_item_clicked(QListWidgetItem * item);
	void on_item_right_clicked(QListWidgetItem * item);
	void build_rows();
	void update_all_state();
	void update_styles();

	enum class item_kind_t
	{
		all,
		type,
		sub_type,
		yaml
	};

	struct row_t
	{
		QListWidgetItem * item = nullptr;
		item_kind_t kind = item_kind_t::type;
		rec_type_t type = rec_type_t::unknown;
		std::string sub_type;
		bool selected = true;
	};

	QListWidget * m_list = nullptr;
	std::vector<row_t> m_rows;
	int m_all_row = -1;
	int m_yaml_row = -1;
	bool m_enabled = true;
};
