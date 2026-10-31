#pragma once

#include <io/file_list.hpp>
#include <optional>
#include <string>
#include <QDialog>

class eet_plugin_dialog_t : public QDialog
{
	Q_OBJECT

public:
	eet_plugin_dialog_t(
	    const file_list_t & file_list,
	    const std::string & plugin_name,
	    QWidget * parent = nullptr);

	std::optional<std::string> selected_plugin_path() const;

private:
	void populate_plugin_tree(const std::string & plugin_name);

	const file_list_t & m_file_list;
	std::optional<std::string> m_selected_plugin_path;
};
