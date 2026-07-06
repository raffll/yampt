#pragma once

#include "../model/make_base_params.hpp"
#include <io/file_list.hpp>
#include <optional>
#include <string>
#include <QDialog>

class settings_store_t;

class make_base_dialog_t : public QDialog
{
	Q_OBJECT

public:
	make_base_dialog_t(
	    const file_list_t & file_list,
	    const settings_store_t & settings,
	    const std::string & source_plugin_path,
	    QWidget * parent = nullptr);

	std::optional<make_base_params_t> result() const;

private:
	void populate_plugin_tree(const std::string & source_plugin_path);
	void populate_dictionary_combo();

	const file_list_t & m_file_list;
	const settings_store_t & m_settings;
	std::optional<make_base_params_t> m_result;
};
