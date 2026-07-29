#include "dict_operations_controller.hpp"
#include "../dialog/merge_dialog.hpp"
#include "../model/dict_document.hpp"
#include "../view/log_view.hpp"
#include <io/dict_writer.hpp>
#include <merger/dict_merger.hpp>
#include <utility/app_logger.hpp>
#include <utility/string_utils.hpp>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QTabWidget>

dict_operations_controller_t::dict_operations_controller_t(dict_operations_deps_t deps)
    : m_deps(std::move(deps))
{}

void dict_operations_controller_t::on_merge()
{
	const auto all_dicts = m_deps.session.all_dicts();
	if (all_dicts.size() < 2)
	{
		QMessageBox::information(
		    m_deps.parent_widget,
		    QCoreApplication::translate("yTranslator", "Merge"),
		    QCoreApplication::translate("yTranslator", "At least 2 dictionaries must be loaded to merge."));
		return;
	}

	std::vector<merge_dialog_t::dict_entry_t> loaded_dicts;
	for (const auto * dict_doc : all_dicts)
	{
		auto filename = std::string(string_utils::extract_filename(dict_doc->path()));
		loaded_dicts.push_back({ filename, dict_doc->path(), dict_doc->dict_kind() });
	}

	merge_dialog_t dialog(loaded_dicts, m_deps.parent_widget);
	if (dialog.exec() != QDialog::Accepted)
		return;

	const auto selected_paths = dialog.selected_paths();
	if (selected_paths.size() < 2)
		return;

	app_logger_t::reset_log();

	dict_merger_t merger(selected_paths);
	const auto & merged_dict = merger.get_dict();

	const auto workspace_dir = QCoreApplication::applicationDirPath().toStdString() + "/workspace/";
	QDir().mkpath(QString::fromStdString(workspace_dir));

	const auto output_path =
	    workspace_dir + "Merged_" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss").toStdString() + ".json";

	dict_writer_t::write(merged_dict, output_path);

	const auto log_text = app_logger_t::get_log();
	m_deps.log_view.append_log("merge", log_text);
	m_deps.record_tabs.setCurrentWidget(&m_deps.log_view);

	m_deps.scan_workspace();

	const auto norm_output = string_utils::normalize_path(output_path);
	auto * doc = m_deps.session.find(norm_output);
	if (doc)
	{
		m_deps.switch_document(doc);
		m_deps.rebuild_sidebar();
	}
}
