#include "plugin_operations_controller.hpp"
#include "../model/dict_document.hpp"
#include "../view/display_name.hpp"
#include "../view/log_view.hpp"
#include "../view/translation_suggestion_view.hpp"
#include <merger/dict_merger.hpp>
#include <utility/app_logger.hpp>
#include <utility/string_utils.hpp>
#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QRadioButton>
#include <QTimer>
#include <QTreeWidget>
#include <QVBoxLayout>

plugin_operations_controller_t::plugin_operations_controller_t(plugin_operations_deps_t deps)
    : m_deps(std::move(deps))
{}

void plugin_operations_controller_t::on_plugin_operation(const std::string & plugin_path_arg, plugin_op_t operation)
{
	const auto plugin_path = plugin_path_arg;
	const auto encoding = m_deps.codepage;

	const auto path_sep = plugin_path.find_last_of("/\\");
	auto plugin_dir = path_sep != std::string::npos ? plugin_path.substr(0, path_sep) : std::string {};
	plugin_dir = string_utils::normalize_path(plugin_dir);

	const auto workspace_dir = QCoreApplication::applicationDirPath().toStdString() + "/workspace/";

	if (operation == plugin_op_t::convert || operation == plugin_op_t::create_plugin)
		m_deps.executor.set_output_dir(plugin_dir);
	else
		m_deps.executor.set_output_dir(workspace_dir);

	if (m_deps.session.has_any_unsaved())
	{
		auto answer = QMessageBox::question(
		    m_deps.parent_widget,
		    QCoreApplication::translate("yTranslator", "Unsaved Changes"),
		    QCoreApplication::translate(
		        "yTranslator", "Some dictionaries have unsaved changes. Save before proceeding?"),
		    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

		if (answer == QMessageBox::Cancel)
			return;

		if (answer == QMessageBox::Yes)
			m_deps.session.save_all();
	}

	operation_executor_t::result_t result;

	switch (operation)
	{
	case plugin_op_t::make_dict:
	{
		auto entries = build_dict_entries(plugin_dir);

		dict_selection_dialog_t dialog(entries, m_deps.settings.last_merge_order(), m_deps.parent_widget);
		dialog.setWindowTitle(QCoreApplication::translate("yTranslator", "Select Dictionaries"));
		if (dialog.exec() != QDialog::Accepted)
			return;

		const auto selected = dialog.get_selected_paths();
		if (selected.empty())
		{
			result = m_deps.executor.make_dict(plugin_path, encoding);
		}
		else
		{
			m_deps.settings.set_last_merge_order(selected);

			for (const auto & sel_path : selected)
				m_deps.session.open(sel_path);

			dict_merger_t merger(selected);
			result = m_deps.executor.make_dict_with_base(plugin_path, merger.get_dict(), encoding);
		}
		break;
	}
	case plugin_op_t::make_base:
	{
		auto params = m_deps.callbacks.show_make_base_dialog(plugin_path);
		if (!params.has_value())
			return;

		result = m_deps.executor.make_base(
		    plugin_path,
		    params->native_path,
		    params->foreign_lang,
		    params->native_lang,
		    nullptr,
		    params->base_mode,
		    params->dictionary_aff_path);
		break;
	}
	case plugin_op_t::convert:
	{
		auto entries = build_dict_entries(plugin_dir);

		dict_selection_dialog_t dialog(entries, m_deps.settings.last_merge_order(), m_deps.parent_widget);
		dialog.setWindowTitle(QCoreApplication::translate("yTranslator", "Select Dictionaries for Convert"));
		if (dialog.exec() != QDialog::Accepted)
			return;

		const auto selected = dialog.get_selected_paths();
		if (selected.empty())
			return;

		m_deps.settings.set_last_merge_order(selected);

		for (const auto & sel_path : selected)
			m_deps.session.open(sel_path);

		result = m_deps.executor.convert(plugin_path, selected, encoding);
		break;
	}
	case plugin_op_t::create_plugin:
	{
		auto entries = build_dict_entries(plugin_dir);

		dict_selection_dialog_t dialog(entries, m_deps.settings.last_merge_order(), m_deps.parent_widget);
		dialog.setWindowTitle(QCoreApplication::translate("yTranslator", "Select Dictionaries for Create"));
		if (dialog.exec() != QDialog::Accepted)
			return;

		const auto selected = dialog.get_selected_paths();
		if (selected.empty())
			return;

		m_deps.settings.set_last_merge_order(selected);

		for (const auto & sel_path : selected)
			m_deps.session.open(sel_path);

		result = m_deps.executor.create_plugin(plugin_path, selected, encoding);
		break;
	}
	}

	log_operation_result(plugin_path, operation, result);

	if (!result.output_path.empty())
		m_deps.session.close(result.output_path);

	m_deps.callbacks.scan_workspace();
}

void plugin_operations_controller_t::log_operation_result(
    const std::string & plugin_path,
    plugin_op_t operation_type,
    const operation_executor_t::result_t & result)
{
	const auto sep = plugin_path.find_last_of("/\\");
	const auto plugin_name = sep != std::string::npos ? plugin_path.substr(sep + 1) : plugin_path;

	std::string operation_name;
	switch (operation_type)
	{
	case plugin_op_t::make_dict:
		operation_name = "make dict: " + plugin_name;
		break;
	case plugin_op_t::make_base:
		operation_name = "make base: " + plugin_name;
		break;
	case plugin_op_t::convert:
		operation_name = "convert: " + plugin_name;
		break;
	case plugin_op_t::create_plugin:
		operation_name = "create: " + plugin_name;
		break;
	}

	m_deps.log_view.append_log(operation_name, result.log_text);
	m_deps.record_tabs.setCurrentWidget(&m_deps.log_view);
}

std::vector<dict_selection_dialog_t::dict_entry_t> plugin_operations_controller_t::build_dict_entries(
    const std::string & source_dir) const
{
	std::set<std::string> seen;
	std::vector<dict_selection_dialog_t::dict_entry_t> entries;

	auto normalize = [](std::string_view path) { return string_utils::to_lower(string_utils::normalize_path(path)); };

	auto matches_source_dir = [&](const std::string & norm, const std::string & target)
	{
		if (target.empty())
			return false;

		const auto dir_sep = norm.find_last_of('/');
		if (dir_sep == std::string::npos)
			return false;

		return norm.substr(0, dir_sep) == target;
	};

	const auto target = source_dir.empty() ? std::string {} : normalize(source_dir);

	std::set<std::string> saved_order_set;
	for (const auto & path : m_deps.settings.last_merge_order())
		saved_order_set.insert(normalize(path));

	const bool use_saved = !saved_order_set.empty();

	for (const auto * dict_doc : m_deps.session.all_dicts())
	{
		const auto norm = normalize(dict_doc->path());
		if (!seen.insert(norm).second)
			continue;

		const bool pre = use_saved ? (saved_order_set.count(norm) > 0) : matches_source_dir(norm, target);

		std::string root_path;
		std::string subfolder;
		const auto * file_entry = m_deps.file_list.get(dict_doc->path());
		if (file_entry)
		{
			root_path = file_entry->root_path;
			subfolder = file_entry->workspace_subfolder;
		}

		entries.push_back(
		    { std::string(string_utils::extract_filename(dict_doc->path())),
		      dict_doc->path(),
		      dict_doc->dict_kind(),
		      root_path,
		      subfolder,
		      pre });
	}

	for (const auto * file_entry : m_deps.file_list.all())
	{
		if (file_entry->type != file_type_t::user_dict && file_entry->type != file_type_t::base_dict)
			continue;

		const auto norm = normalize(file_entry->path);
		if (!seen.insert(norm).second)
			continue;

		const bool pre = use_saved ? (saved_order_set.count(norm) > 0) : matches_source_dir(norm, target);
		const auto kind = (file_entry->type == file_type_t::base_dict) ? dict_kind_t::base : dict_kind_t::user;
		entries.push_back(
		    { file_entry->filename,
		      file_entry->path,
		      kind,
		      file_entry->root_path,
		      file_entry->workspace_subfolder,
		      pre });
	}

	return entries;
}
