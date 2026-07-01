#pragma once

#include "../model/table_builder.hpp"
#include <utility/dict_kind.hpp>
#include <string>
#include <vector>

class QAbstractButton;
class QLabel;
class QLineEdit;

class filter_tree_view_t;
class record_table_model_t;
class status_filter_view_t;

class table_view_t
{
public:
	table_view_t(
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
	    QAbstractButton & col_trans);

	void apply(table_build_result_t result, const std::string & file_path, dict_kind_t kind);
	void apply_yaml(
	    std::vector<table_row_t> rows,
	    int total,
	    int translated,
	    const std::string & file_path,
	    const std::map<status_t, size_t> & filtered_status_counts,
	    const std::map<status_t, size_t> & total_status_counts);
	void clear();
	void set_enabled(bool enabled);

private:
	filter_tree_view_t & m_filter_tree;
	status_filter_view_t & m_status_bar;
	record_table_model_t & m_model;
	QLabel & m_progress;
	QLabel & m_file_label;
	QLabel & m_search_label;
	QLineEdit & m_search_field;
	QAbstractButton & m_case_check;
	QAbstractButton & m_regex_check;
	QAbstractButton & m_col_key;
	QAbstractButton & m_col_orig;
	QAbstractButton & m_col_trans;
};
