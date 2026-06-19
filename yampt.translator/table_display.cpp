#include "table_display.hpp"
#include "filter_tree.hpp"
#include "record_table_model.hpp"
#include "status_filter_bar.hpp"

#include <QAbstractButton>
#include <QLabel>
#include <QLineEdit>
#include <QString>

table_display_t::table_display_t(
    filter_tree_t & filter_tree,
    status_filter_bar_t & status_bar,
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
    : filter_tree_(filter_tree)
    , status_bar_(status_bar)
    , model_(model)
    , progress_(progress)
    , file_label_(file_label)
    , search_label_(search_label)
    , search_field_(search_field)
    , case_check_(case_check)
    , regex_check_(regex_check)
    , col_key_(col_key)
    , col_orig_(col_orig)
    , col_trans_(col_trans)
{}

void table_display_t::apply(table_build_result_t result, const std::string & file_path, dict_kind_t kind)
{
	file_label_.setText(QString::fromStdString(file_path));
	filter_tree_.setEnabled(true);
	filter_tree_.set_display_mode(filter_tree_t::display_mode_t::full);
	set_enabled(true);

	if (kind == dict_kind_t::base)
		status_bar_.set_dict_mode(status_filter_bar_t::dict_mode_t::base);
	else
		status_bar_.set_dict_mode(status_filter_bar_t::dict_mode_t::user);

	filter_tree_.update_counts(result.counts.type_counts, result.counts.translated_counts);
	filter_tree_.update_sub_type_counts(result.counts.sub_type_total_counts, result.counts.sub_type_translated_counts);

	size_t total = 0;
	size_t total_translated = 0;
	for (const auto & [t, c] : result.counts.type_counts)
		total += c;
	for (const auto & [t, c] : result.counts.translated_counts)
		total_translated += c;

	filter_tree_.set_total_count(total_translated, total);
	status_bar_.update_counts(result.counts.filtered_status_counts, result.counts.total_status_counts);

	model_.rebuild(std::move(result.rows));

	if (result.counts.progress_total > 0)
	{
		int pct = static_cast<int>(result.counts.progress_translated * 100 / result.counts.progress_total);
		int shown = model_.rowCount();
		progress_.setText(QString("%1 / %2 (%3%) | %4 shown")
		                      .arg(result.counts.progress_translated)
		                      .arg(result.counts.progress_total)
		                      .arg(pct)
		                      .arg(shown));
	}
	else
	{
		progress_.clear();
	}
}

void table_display_t::apply_yaml(
    std::vector<table_row_t> rows,
    int total,
    int translated,
    const std::string & file_path,
    const std::map<std::string, size_t> & filtered_status_counts,
    const std::map<std::string, size_t> & total_status_counts)
{
	file_label_.setText(QString::fromStdString(file_path));
	filter_tree_.setEnabled(true);
	filter_tree_.set_display_mode(filter_tree_t::display_mode_t::all_only);
	status_bar_.set_dict_mode(status_filter_bar_t::dict_mode_t::user);
	set_enabled(true);

	status_bar_.update_counts(filtered_status_counts, total_status_counts);

	model_.rebuild(std::move(rows));

	if (total > 0)
	{
		int pct = translated * 100 / total;
		int shown = model_.rowCount();
		progress_.setText(QString("%1 / %2 (%3%) | %4 shown").arg(translated).arg(total).arg(pct).arg(shown));
	}
	else
	{
		progress_.clear();
	}
}

void table_display_t::clear()
{
	model_.rebuild({});
	progress_.clear();
	file_label_.clear();
	filter_tree_.setEnabled(false);
	status_bar_.set_dict_mode(status_filter_bar_t::dict_mode_t::none);
	set_enabled(false);
}

void table_display_t::set_enabled(bool enabled)
{
	search_label_.setEnabled(enabled);
	search_field_.setEnabled(enabled);
	case_check_.setEnabled(enabled);
	regex_check_.setEnabled(enabled);
	col_key_.setEnabled(enabled);
	col_orig_.setEnabled(enabled);
	col_trans_.setEnabled(enabled);
}
