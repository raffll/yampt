#pragma once

#include "table_builder.hpp"
#include "../yampt/dict_kind.hpp"

#include <string>
#include <vector>

class QAbstractButton;
class QLabel;
class QLineEdit;

class filter_tree_t;
class record_table_model_t;
class status_filter_bar_t;

class table_display_t
{
public:
	table_display_t(
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
	    QAbstractButton & col_trans);

	void apply(table_build_result_t result, const std::string & file_path, dict_kind_t kind);
	void apply_yaml(
	    std::vector<table_row_t> rows,
	    int total,
	    int translated,
	    const std::string & file_path,
	    const std::map<std::string, size_t> & filtered_status_counts,
	    const std::map<std::string, size_t> & total_status_counts);
	void clear();
	void set_enabled(bool enabled);

private:
	filter_tree_t & filter_tree_;
	status_filter_bar_t & status_bar_;
	record_table_model_t & model_;
	QLabel & progress_;
	QLabel & file_label_;
	QLabel & search_label_;
	QLineEdit & search_field_;
	QAbstractButton & case_check_;
	QAbstractButton & regex_check_;
	QAbstractButton & col_key_;
	QAbstractButton & col_orig_;
	QAbstractButton & col_trans_;
};
