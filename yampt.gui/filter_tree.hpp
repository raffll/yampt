#pragma once

#include "../yampt/tools.hpp"
#include <QWidget>
#include <map>
#include <set>
#include <string>
#include <vector>

class QTreeWidget;
class QTreeWidgetItem;

class filter_tree_t : public QWidget
{
	Q_OBJECT

public:
	explicit filter_tree_t(QWidget * parent = nullptr);

	enum class display_mode_t { full, all_only, empty };
	void set_display_mode(display_mode_t mode);

	void update_counts(const std::map<tools_t::rec_type_t, size_t> & total_counts,
		const std::map<tools_t::rec_type_t, size_t> & translated_counts);
	void update_sub_type_counts(const std::map<std::string, size_t> & sub_type_total_counts,
		const std::map<std::string, size_t> & sub_type_translated_counts);
	void set_total_count(size_t translated, size_t total);

	std::set<tools_t::rec_type_t> get_active_types() const;
	std::set<std::string> get_active_sub_types() const;
	bool has_sub_type_filter() const;
	void set_active_types(const std::set<tools_t::rec_type_t> & types);
	void set_active_sub_types(const std::set<std::string> & sub_types);
	bool is_solo() const;
	tools_t::rec_type_t get_solo_type() const;

	bool is_yaml_filter_active() const;
	void set_yaml_filter_active(bool active);
	void set_yaml_button_visible(bool visible);
	void setEnabled(bool enabled);

signals:
	void filters_changed();
	void all_reset_requested();

private:
	enum class node_state_t { selected, partial, deselected };

	static constexpr int role_state = Qt::UserRole + 10;
	static constexpr int role_type = Qt::UserRole + 11;
	static constexpr int role_sub_type = Qt::UserRole + 12;
	static constexpr int role_is_all = Qt::UserRole + 13;
	static constexpr int role_is_yaml = Qt::UserRole + 14;

	void on_item_clicked(QTreeWidgetItem * item, int column);
	void on_item_right_clicked(QTreeWidgetItem * item);
	void set_subtree_selected(QTreeWidgetItem * item, bool selected);
	void update_parent_state(QTreeWidgetItem * parent);
	void update_all_node_state();
	void update_item_styles();
	void apply_item_style(QTreeWidgetItem * item);

	struct type_node_t
	{
		QTreeWidgetItem * item = nullptr;
		tools_t::rec_type_t type;
	};

	struct sub_type_node_t
	{
		QTreeWidgetItem * item = nullptr;
		std::string sub_type;
		tools_t::rec_type_t parent_type;
	};

	QTreeWidget * tree_ = nullptr;
	QTreeWidgetItem * all_item_ = nullptr;
	QTreeWidgetItem * yaml_item_ = nullptr;

	std::vector<type_node_t> type_nodes_;
	std::vector<sub_type_node_t> sub_type_nodes_;
	bool enabled_ = true;
};
