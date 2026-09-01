#include <resource_paths.hpp>
#include "dict_operations_controller.hpp"
#include "../dialog/merge_dialog.hpp"
#include "../editor/edit_history.hpp"
#include "../model/dict_document.hpp"
#include "../view/log_view.hpp"
#include <creator/topic_tagger.hpp>
#include <io/dict_writer.hpp>
#include <merger/dict_merger.hpp>
#include <utility/app_logger.hpp>
#include <utility/string_utils.hpp>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QTabWidget>

namespace
{
constexpr std::size_t minimum_tag_length = 3;

bool is_whitespace_only(const std::string & text)
{
	return text.empty() || text.find_first_not_of(" \t\r\n") == std::string::npos;
}

bool has_control_characters(const std::string & text)
{
	for (const auto character : text)
	{
		const auto value = static_cast<unsigned char>(character);
		if (value < 0x20 && value != '\t' && value != '\r' && value != '\n')
			return true;
	}

	return false;
}

bool should_skip_record(const record_entry_t & entry)
{
	if (entry.status != status_t::translated)
		return true;

	if (is_whitespace_only(entry.new_text))
		return true;

	if (entry.new_text.size() < minimum_tag_length)
		return true;

	if (has_control_characters(entry.old_text))
		return true;

	if (has_control_characters(entry.new_text))
		return true;

	return false;
}

std::string build_summary(const apply_tags_result_t & result)
{
	app_logger_t::reset_log();
	app_logger_t::add_log(
	    "[info] apply tags: " + std::to_string(result.entries_changed) + " entries changed, "
	    + std::to_string(result.tags_inserted) + " tags inserted\r\n");

	return app_logger_t::get_log();
}

struct tag_context_t
{
	dict_document_t & document;
	edit_history_t & edit_history;
	const topic_tagger_t & tagger;
};

struct record_location_t
{
	rec_type_t type;
	std::size_t index;
};

apply_tags_result_t tag_record(tag_context_t context, const record_location_t & location)
{
	auto & chapter = context.document.data_mut().at(location.type);
	auto & entry = chapter.records.at(location.index);
	if (should_skip_record(entry))
		return {};

	const auto tagged = context.tagger.tag_line(entry.new_text);
	if (tagged.text == entry.new_text)
		return {};

	context.edit_history.record_change(location.type, entry.key_text, entry.new_text, tagged.text, entry.status);

	entry.new_text = tagged.text;
	context.document.modified_records_insert(location.type, location.index);
	context.document.set_dirty(true);

	return { 1, tagged.tags_inserted };
}

apply_tags_result_t tag_all_records(tag_context_t context)
{
	apply_tags_result_t total;

	for (const auto & chapter_pair : context.document.data())
	{
		const auto record_count = chapter_pair.second.records.size();
		for (std::size_t index = 0; index < record_count; ++index)
		{
			const auto partial = tag_record(context, { chapter_pair.first, index });
			total.entries_changed += partial.entries_changed;
			total.tags_inserted += partial.tags_inserted;
		}
	}

	return total;
}
} // namespace

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

	const auto workspace_dir = resource_paths::workspace_dir();
	QDir().mkpath(QString::fromStdString(workspace_dir));

	const auto merged_filename =
	    "Merged_" + QDateTime::currentDateTime().toString("yyyyMMddHHmmss").toStdString() + ".json";
	const auto output_path = string_utils::join_path(workspace_dir, merged_filename);

	dict_writer_t::write(merged_dict, output_path);

	const auto log_text = app_logger_t::get_log();
	m_deps.log_view.append_log("merge", log_text);
	m_deps.record_tabs.setCurrentWidget(&m_deps.log_view);

	m_deps.scan_workspace();

	const auto norm_output = string_utils::canonicalize_path(output_path);
	auto * doc = m_deps.session.find(norm_output);
	if (doc)
	{
		m_deps.switch_document(doc);
		m_deps.rebuild_sidebar();
	}
}

void dict_operations_controller_t::on_apply_tags(dict_document_t * dict_doc)
{
	if (!dict_doc)
		return;

	topic_tagger_t tagger;
	tagger.seed_topics(dict_doc->data());

	const auto result = tag_all_records({ *dict_doc, m_deps.edit_history, tagger });

	if (result.entries_changed > 0 && m_deps.refresh_table)
		m_deps.refresh_table();

	m_deps.log_view.append_log("apply tags", build_summary(result));
	m_deps.record_tabs.setCurrentWidget(&m_deps.log_view);
}
