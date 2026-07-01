#include "table_view.hpp"
#include "../model/record_table_model.hpp"
#include "filter_tree_view.hpp"
#include "status_filter_view.hpp"
#include <QAbstractButton>
#include <QLabel>
#include <QLineEdit>
#include <QString>

table_view_t::table_view_t(
    filter_tree_view_t & filter_tree,
    status_filter_view_t & status_bar,
    record_table_model_t & model,
    QLabel & progress,
    QLabel & file_label,
    QLabel & search_label,
    QLineEdit & search_field,
    QAbstractButton & case_check,
    QAbstractButton & regex_check,
    QAbstractButton & col_key,
    QAbstractButton & col_orig,
    QAbstractButton & col_trans)
    : m_filter_tree(filter_tree)
    , m_status_bar(status_bar)
    , m_model(model)
    , m_progress(progress)
    , m_file_label(file_label)
    , m_search_label(search_label)
    , m_search_field(search_field)
    , m_case_check(case_check)
    , m_regex_check(regex_check)
    , m_col_key(col_key)
    , m_col_orig(col_orig)
    , m_col_trans(col_trans)
{}

void table_view_t::apply(table_build_result_t result, const std::string & file_path, dict_kind_t kind)
{
	m_file_label.setText(QString::fromStdString(file_path));
	m_filter_tree.setEnabled(true);
	m_filter_tree.set_display_mode(filter_tree_view_t::display_mode_t::full);
	set_enabled(true);

	m_status_bar.set_document_open(true);

	m_filter_tree.update_counts(result.counts.type_counts, result.counts.translated_counts);
	m_filter_tree.update_sub_type_counts(result.counts.sub_type_total_counts, result.counts.sub_type_translated_counts);

	size_t total = 0;
	size_t total_translated = 0;
	for (const auto & [t, c] : result.counts.type_counts)
		total += c;
	for (const auto & [t, c] : result.counts.translated_counts)
		total_translated += c;

	m_filter_tree.set_total_count(total_translated, total);
	m_status_bar.update_counts(result.counts.filtered_status_counts, result.counts.total_status_counts);

	m_model.rebuild(std::move(result.rows));

	if (result.counts.progress_total > 0)
	{
		int pct = static_cast<int>(result.counts.progress_translated * 100 / result.counts.progress_total);
		int shown = m_model.rowCount();
		m_progress.setText(QString("%1 / %2 (%3%) | %4 shown")
		                      .arg(result.counts.progress_translated)
		                      .arg(result.counts.progress_total)
		                      .arg(pct)
		                      .arg(shown));
	}
	else
	{
		m_progress.clear();
	}
}

void table_view_t::apply_yaml(
    std::vector<table_row_t> rows,
    int total,
    int translated,
    const std::string & file_path,
    const std::map<status_t, size_t> & filtered_status_counts,
    const std::map<status_t, size_t> & total_status_counts)
{
	m_file_label.setText(QString::fromStdString(file_path));
	m_filter_tree.setEnabled(true);
	m_filter_tree.set_display_mode(filter_tree_view_t::display_mode_t::all_only);
	m_status_bar.set_document_open(true);
	set_enabled(true);

	m_status_bar.update_counts(filtered_status_counts, total_status_counts);

	m_model.rebuild(std::move(rows));

	if (total > 0)
	{
		int pct = translated * 100 / total;
		int shown = m_model.rowCount();
		m_progress.setText(QString("%1 / %2 (%3%) | %4 shown").arg(translated).arg(total).arg(pct).arg(shown));
	}
	else
	{
		m_progress.clear();
	}
}

void table_view_t::clear()
{
	m_model.rebuild({});
	m_progress.clear();
	m_file_label.clear();
	m_filter_tree.setEnabled(false);
	m_status_bar.set_document_open(false);
	set_enabled(false);
}

void table_view_t::set_enabled(bool enabled)
{
	m_search_label.setEnabled(enabled);
	m_search_field.setEnabled(enabled);
	m_case_check.setEnabled(enabled);
	m_regex_check.setEnabled(enabled);
	m_col_key.setEnabled(enabled);
	m_col_orig.setEnabled(enabled);
	m_col_trans.setEnabled(enabled);
}
