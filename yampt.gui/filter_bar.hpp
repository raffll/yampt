#pragma once

#include "../yampt/tools.hpp"
#include <QWidget>
#include <map>
#include <set>
#include <string>
#include <vector>

class QPushButton;
class QHBoxLayout;

class filter_bar_t : public QWidget
{
	Q_OBJECT

public:
	explicit filter_bar_t(QWidget * parent = nullptr);

	void update_counts(const std::map<tools_t::rec_type_t, size_t> & counts);
	std::set<tools_t::rec_type_t> get_active_types() const;
	std::set<std::string> get_active_sub_types() const;
	bool is_solo() const;
	tools_t::rec_type_t get_solo_type() const;

	void set_filter_state(const std::set<tools_t::rec_type_t> & types, bool solo, tools_t::rec_type_t solo_type);

signals:
	void filters_changed();

private:
	void on_type_clicked(tools_t::rec_type_t type);
	void on_type_right_clicked(tools_t::rec_type_t type);
	void on_all_clicked();
	void update_button_styles();
	void rebuild_sub_type_bar();
	void update_sub_type_styles();
	void on_sub_type_clicked(const std::string & value);
	void on_sub_type_right_clicked(const std::string & value);

	struct type_button_t
	{
		QPushButton * button = nullptr;
		tools_t::rec_type_t type;
		size_t count = 0;
	};

	QPushButton * all_button_ = nullptr;
	std::vector<type_button_t> type_buttons_;
	std::set<tools_t::rec_type_t> active_types_;
	std::set<tools_t::rec_type_t> saved_types_;
	bool solo_ = false;
	tools_t::rec_type_t solo_type_ = tools_t::rec_type_t::unknown;

	QWidget * sub_type_bar_ = nullptr;
	QHBoxLayout * sub_type_layout_ = nullptr;
	std::vector<QPushButton *> sub_type_buttons_;
	std::set<std::string> active_sub_types_;
	std::set<std::string> saved_sub_types_;
	bool sub_type_solo_ = false;
	std::string sub_type_solo_value_;
};
