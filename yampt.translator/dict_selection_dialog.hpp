#pragma once

#include "../yampt/dict_kind.hpp"

#include <QDialog>
#include <string>
#include <vector>

class QDialogButtonBox;
class QListWidget;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

class dict_selection_dialog_t : public QDialog
{
	Q_OBJECT

public:
	struct dict_entry_t
	{
		std::string name;
		std::string path;
		dict_kind_t kind = dict_kind_t::user;
		std::string root_path;
		std::string subfolder;
		bool checked = false;
	};

	explicit dict_selection_dialog_t(
	    const std::vector<dict_entry_t> & entries,
	    const std::vector<std::string> & saved_order = {},
	    QWidget * parent = nullptr);

	std::vector<std::string> get_selected_paths() const;

private:
	void on_tree_item_changed(QTreeWidgetItem * item, int column);
	void on_move_up();
	void on_move_down();
	void update_ok_button();

	QTreeWidget * tree_ = nullptr;
	QListWidget * order_list_ = nullptr;
	QPushButton * up_button_ = nullptr;
	QPushButton * down_button_ = nullptr;
	QDialogButtonBox * button_box_ = nullptr;
};
