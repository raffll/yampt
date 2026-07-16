#include "sidebar_controller.hpp"
#include "../model/dict_document.hpp"
#include "../model/sidebar_model.hpp"
#include "../model/yaml_document.hpp"
#include "../view/display_name.hpp"
#include "../view/log_view.hpp"
#include "../view/sidebar_view.hpp"
#include <utility/string_utils.hpp>
#include <algorithm>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QMessageBox>

sidebar_controller_t::sidebar_controller_t(sidebar_controller_deps_t deps)
    : m_deps(std::move(deps))
{}

void sidebar_controller_t::on_item_clicked(const std::string & path)
{
	const auto * entry = m_deps.file_list.get(path);
	if (!entry)
		return;

	const auto norm_path = string_utils::normalize_path(path);
	if (m_deps.active_doc && m_deps.active_doc->path() == norm_path)
		return;

	auto * doc = m_deps.session.open(path);
	if (!doc)
		return;

	m_deps.callbacks.switch_document(doc);
	rebuild_sidebar();
}

void sidebar_controller_t::on_save_requested(const std::string & path)
{
	const auto norm_path = string_utils::normalize_path(path);

	if (m_deps.active_doc && m_deps.active_doc->path() == norm_path)
	{
		m_deps.active_doc->save();
		update_sidebar_item(m_deps.active_doc->path());
		m_deps.log_view.append_log("save", "saved \"" + m_deps.active_doc->path() + "\"\r\n");

		if (!m_deps.session.has_any_unsaved())
			m_deps.callbacks.set_unsaved_changes(false);

		return;
	}

	auto * doc = m_deps.session.find(path);
	if (!doc)
		return;

	doc->save();
	update_sidebar_item(doc->path());
	m_deps.log_view.append_log("save", "saved \"" + doc->path() + "\"\r\n");

	if (!m_deps.session.has_any_unsaved())
		m_deps.callbacks.set_unsaved_changes(false);
}

void sidebar_controller_t::on_unload_requested(const std::string & path)
{
	auto * doc = m_deps.session.find(path);
	if (!doc)
	{
		m_deps.file_list.remove(path);
		rebuild_sidebar();
		return;
	}

	if (doc->is_dirty())
	{
		auto answer = QMessageBox::question(
		    m_deps.parent_widget,
		    "Unsaved Changes",
		    "This dictionary has unsaved changes. Save before unloading?",
		    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

		if (answer == QMessageBox::Cancel)
			return;

		if (answer == QMessageBox::Save)
			doc->save();
	}

	if (m_deps.active_doc && m_deps.active_doc->path() == doc->path())
		m_deps.callbacks.switch_document(nullptr);

	m_deps.session.close(path);

	const auto norm = string_utils::normalize_path(path);
	m_deps.filter_states.erase(norm);

	m_deps.callbacks.rebuild_annotations();
	m_deps.last_annotation_version = m_deps.session.dict_version();
	rebuild_sidebar();
}

void sidebar_controller_t::on_delete_requested(const std::string & path)
{
	const auto sep = path.find_last_of("/\\");
	const auto filename = sep != std::string::npos ? path.substr(sep + 1) : path;

	auto answer = QMessageBox::question(
	    m_deps.parent_widget,
	    "Delete File",
	    QString("Delete \"%1\" from disk?").arg(QString::fromStdString(filename)),
	    QMessageBox::Yes | QMessageBox::No);

	if (answer != QMessageBox::Yes)
		return;

	if (!QFile::remove(QString::fromStdString(path)))
	{
		QMessageBox::warning(
		    m_deps.parent_widget,
		    QCoreApplication::translate("yTranslator", "Error"),
		    QString("Failed to delete \"%1\".").arg(QString::fromStdString(filename)));
		return;
	}

	const auto norm_del_path = string_utils::normalize_path(path);
	if (m_deps.active_doc && m_deps.active_doc->path() == norm_del_path)
		m_deps.callbacks.switch_document(nullptr);

	m_deps.session.close(path);
	m_deps.filter_states.erase(norm_del_path);
	m_deps.callbacks.rebuild_annotations();
	m_deps.last_annotation_version = m_deps.session.dict_version();
	m_deps.file_list.remove(path);
	rebuild_sidebar();
	scan_workspace();
}

void sidebar_controller_t::scan_workspace()
{
	const auto workspace = (QCoreApplication::applicationDirPath() + "/workspace").toStdString();
	QDir().mkpath(QString::fromStdString(workspace));

	std::vector<std::string> roots;
	roots.push_back(workspace);
	for (const auto & root : m_deps.file_list.get_roots())
	{
		if (root != workspace)
			roots.push_back(root);
	}

	m_deps.file_list.scan_roots(roots);

	bool any_loaded = false;
	for (const auto * entry : m_deps.file_list.all())
	{
		if (!entry->is_workspace)
			continue;

		if (entry->type != file_type_t::base_dict && entry->type != file_type_t::user_dict &&
		    entry->type != file_type_t::yaml_l10n)
			continue;

		if (m_deps.session.find(entry->path))
			continue;

		if (m_deps.session.open(entry->path))
			any_loaded = true;
	}

	if (any_loaded)
	{
		m_deps.callbacks.rebuild_annotations();
		m_deps.last_annotation_version = m_deps.session.dict_version();
	}

	rebuild_sidebar();
}

void sidebar_controller_t::update_watcher_roots()
{
	QStringList roots;

	const auto workspace = QCoreApplication::applicationDirPath() + "/workspace";
	roots.append(workspace);

	for (const auto & root : m_deps.file_list.get_roots())
		roots.append(QString::fromStdString(root));

	m_deps.workspace_watcher.set_watch_roots(roots);
}

void sidebar_controller_t::rebuild_sidebar()
{
	std::string active_path;
	if (m_deps.active_doc)
		active_path = m_deps.active_doc->path();

	auto model = build_render_model(m_deps.file_list, m_deps.session, active_path);
	m_deps.sidebar_view.set_model(model);
}

void sidebar_controller_t::update_sidebar_item(const std::string & path)
{
	const auto * entry = m_deps.file_list.get(path);
	if (!entry)
		return;

	const auto * doc = m_deps.session.find(path);
	const bool is_loaded = (doc != nullptr);
	const bool is_dirty = doc && doc->is_dirty();
	m_deps.sidebar_view.update_item_text(entry->path, derive_display_name(*entry, is_loaded, is_dirty));
}

void sidebar_controller_t::on_export_native_requested(const std::string & path)
{
	auto * doc = m_deps.session.find(path);
	if (!doc)
		doc = m_deps.session.open(path);

	auto * yaml_doc = dynamic_cast<yaml_document_t *>(doc);
	if (!yaml_doc || yaml_doc->is_native_file())
		return;

	yaml_doc->export_native();
	scan_workspace();
	m_deps.log_view.append_log("export", "created \"" + yaml_doc->native_path() + "\"\r\n");
}

void sidebar_controller_t::on_remove_folder_requested(const std::string & root_path)
{
	auto roots = m_deps.file_list.get_roots();
	roots.erase(std::remove(roots.begin(), roots.end(), root_path), roots.end());
	m_deps.file_list.scan_roots(roots);

	if (m_deps.active_doc)
	{
		const auto * entry = m_deps.file_list.get(m_deps.active_doc->path());
		if (!entry)
			m_deps.callbacks.switch_document(nullptr);
	}

	m_deps.session.close_if([this, &root_path](const document_t & doc)
	{
		if (doc.path().find(root_path) == 0)
		{
			m_deps.filter_states.erase(doc.path());
			return true;
		}

		return false;
	});

	m_deps.callbacks.rebuild_annotations();
	m_deps.last_annotation_version = m_deps.session.dict_version();
	scan_workspace();
	m_deps.callbacks.save_config();
	update_watcher_roots();
}

void sidebar_controller_t::on_delete_folder_requested(const std::string & folder_path)
{
	const auto sep = folder_path.find_last_of("/\\");
	const auto folder_name = sep != std::string::npos ? folder_path.substr(sep + 1) : folder_path;

	auto answer = QMessageBox::question(
	    m_deps.parent_widget,
	    "Delete Folder",
	    QString("Delete \"%1\" and all its contents from disk?").arg(QString::fromStdString(folder_name)),
	    QMessageBox::Yes | QMessageBox::No);

	if (answer != QMessageBox::Yes)
		return;

	if (m_deps.active_doc)
	{
		const auto folder_norm = string_utils::normalize_path(folder_path);
		const auto & doc_path = m_deps.active_doc->path();
		if (doc_path.find(folder_norm + "/") == 0 || doc_path == folder_norm)
			m_deps.callbacks.switch_document(nullptr);
	}

	const auto folder_norm = string_utils::normalize_path(folder_path);

	m_deps.session.close_if([this, &folder_norm](const document_t & doc)
	{
		const auto & doc_path = doc.path();
		if (doc_path.find(folder_norm + "/") == 0 || doc_path == folder_norm)
		{
			m_deps.filter_states.erase(doc_path);
			return true;
		}

		return false;
	});

	m_deps.callbacks.rebuild_annotations();
	m_deps.last_annotation_version = m_deps.session.dict_version();
	QDir(QString::fromStdString(folder_path)).removeRecursively();
	scan_workspace();
}
