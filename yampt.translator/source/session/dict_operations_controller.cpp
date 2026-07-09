#include "dict_operations_controller.hpp"
#include "../dialog/merge_dialog.hpp"
#include "../translator/ctranslate2_translator.hpp"
#include "../view/log_view.hpp"
#include "../view/translation_suggestion_view.hpp"
#include <io/dict_writer.hpp>
#include <merger/dict_merger.hpp>
#include <utility/app_logger.hpp>
#include <utility/string_utils.hpp>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QTabWidget>
#include <QTimer>

dict_operations_controller_t::dict_operations_controller_t(dict_operations_deps_t deps)
    : m_deps(std::move(deps))
{}

void dict_operations_controller_t::on_merge()
{
	const auto all_dicts = m_deps.session.all_dicts();
	if (all_dicts.size() < 2)
	{
		QMessageBox::information(m_deps.parent_widget, "Merge", "At least 2 dictionaries must be loaded to merge.");
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

void dict_operations_controller_t::start_batch_translation(dict_document_t * dict_doc)
{
	m_deps.translation_view.set_batch_in_progress(true);
	m_deps.translation_view.append_log("[info] collecting untranslated entries...\n");

	struct batch_state_t
	{
		std::vector<std::pair<rec_type_t, size_t>> work_items;
		size_t current = 0;
		int translated_count = 0;
		int glossary_count = 0;
		int error_count = 0;
	};

	auto state = std::make_shared<batch_state_t>();
	auto * provider = m_deps.translation_view.ct2_provider();

	auto & data = dict_doc->data_mut();
	for (auto & [type, chapter] : data)
	{
		for (size_t i = 0; i < chapter.records.size(); ++i)
		{
			if (chapter.records[i].status == status_t::untranslated && !chapter.records[i].old_text.empty())
				state->work_items.push_back({ type, i });

			if (state->work_items.size() >= 10)
				break;
		}

		if (state->work_items.size() >= 10)
			break;
	}

	if (state->work_items.empty())
	{
		m_deps.translation_view.append_log("[info] no untranslated entries found\n");
		m_deps.translation_view.set_batch_in_progress(false);
		return;
	}

	m_deps.translation_view.append_log("[info] translating " + std::to_string(state->work_items.size()) + " entries\n");

	auto * timer = new QTimer(m_deps.parent_widget);
	timer->setInterval(0);
	QObject::connect(
	    timer,
	    &QTimer::timeout,
	    m_deps.parent_widget,
	    [this, state, dict_doc, provider, timer]()
	{
		if (state->current >= state->work_items.size())
		{
			timer->stop();
			timer->deleteLater();

			dict_doc->set_dirty(true);
			m_deps.set_unsaved_changes(true);
			m_deps.update_sidebar_item(dict_doc->path());

			if (m_deps.current_row() >= 0)
				m_deps.load_record(m_deps.current_row());

			m_deps.translation_view.append_log(
			    "[info] done: translated=" + std::to_string(state->translated_count) + " glossary=" +
			    std::to_string(state->glossary_count) + " errors=" + std::to_string(state->error_count) + "\n");
			m_deps.translation_view.set_batch_in_progress(false);
			return;
		}

		const auto & [type, idx] = state->work_items[state->current];
		auto & data_ref = dict_doc->data_mut();
		auto type_it = data_ref.find(type);
		if (type_it == data_ref.end() || idx >= type_it->second.records.size())
		{
			++state->current;
			return;
		}

		auto & record = type_it->second.records[idx];
		auto result = provider->translate_sync(record.old_text);

		if (!result.success)
		{
			m_deps.translation_view.append_log(
			    "[error] \"" + record.old_text.substr(0, 40) + "...\" -> " + result.error + "\n");
			++state->error_count;
			++state->current;
			return;
		}

		auto glossary_applied = m_deps.glossary.apply_glossary(result.text);

		record.new_text = glossary_applied;
		record.status = status_t::model;
		dict_doc->modified_records_insert(type, idx);
		++state->translated_count;
		++state->current;
	});
	timer->start();
}
