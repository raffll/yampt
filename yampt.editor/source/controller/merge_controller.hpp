#pragma once

#include "../model/view_tree_model.hpp"
#include <functional>
#include <string>

class plugin_session_t;
class record_view_t;
class nav_tree_view_t;
class settings_store_t;
class QModelIndex;

class merge_controller_t
{
public:
	using log_fn_t = std::function<void(const std::string &)>;

	merge_controller_t(
	    plugin_session_t & session,
	    record_view_t & record_view,
	    nav_tree_view_t & nav_view,
	    settings_store_t & settings,
	    log_fn_t log_fn);

	void create_merged_patch();
	void save_plugin();
	void load_existing_merged_patch();
	std::string resolve_output_directory() const;

	void copy_whole_record(int plugin_idx, const std::string & rec_type, const std::string & record_id);
	void copy_cell_record(
	    int plugin_idx,
	    const std::string & rec_type,
	    const std::string & record_id,
	    const QModelIndex & clicked_index,
	    int clicked_col);
	void copy_sub_record(
	    int plugin_idx,
	    const std::string & rec_type,
	    const std::string & record_id,
	    const std::string & sub_type,
	    int binary_idx);
	void copy_group(int plugin_idx, const std::string & rec_type, const std::string & record_id, int group_row_idx);
	void copy_field(
	    int plugin_idx,
	    const std::string & rec_type,
	    const std::string & record_id,
	    const std::string & sub_type,
	    size_t sub_size,
	    int binary_idx,
	    int field_idx);

	void remove_sub_record(
	    const std::string & rec_type,
	    const std::string & record_id,
	    int binary_idx,
	    const std::string & removed_type);
	void remove_group(
	    const std::string & rec_type,
	    const std::string & record_id,
	    view_tree_model_t::binary_range_t range);

	void remove_record_from_merge(const std::string & rec_type, const std::string & record_id);

private:
	int create_merge_records();
	std::string resolve_merge_output_path() const;
	void save_merged_patch();
	bool save_merge_to_file(
	    const std::string & output_path,
	    const std::string & author,
	    const std::string & description);
	void refresh_after_merge(const std::string & rec_type, const std::string & record_id);

	std::string read_source_content(int plugin_idx, const std::string & rec_type, const std::string & record_id);
	std::string ensure_merge_record(
	    int plugin_idx,
	    const std::string & rec_type,
	    const std::string & record_id,
	    const std::string & source_content);
	int find_plugin_column(int plugin_idx) const;

	plugin_session_t & m_session;
	record_view_t & m_record_view;
	nav_tree_view_t & m_nav_view;
	settings_store_t & m_settings;
	log_fn_t m_log;
};
