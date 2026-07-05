#pragma once

#include <utility/dict_kind.hpp>
#include <string>
#include <vector>
#include <QDialog>

class QDialogButtonBox;
class QListWidget;
class QPushButton;

class merge_dialog_t : public QDialog
{
	Q_OBJECT

public:
	struct dict_entry_t
	{
		std::string name;
		std::string path;
		dict_kind_t kind = dict_kind_t::user;
	};

	explicit merge_dialog_t(const std::vector<dict_entry_t> & loaded_dicts, QWidget * parent = nullptr);

	std::vector<std::string> selected_paths() const;

private:
	void populate_list(const std::vector<dict_entry_t> & loaded_dicts);
	void setup_buttons();
	void connect_signals();
	void update_ok_button();

	void on_move_up();
	void on_move_down();
	void on_remove();

	QListWidget * m_list = nullptr;
	QPushButton * m_up_button = nullptr;
	QPushButton * m_down_button = nullptr;
	QPushButton * m_remove_button = nullptr;
	QDialogButtonBox * m_button_box = nullptr;
};
