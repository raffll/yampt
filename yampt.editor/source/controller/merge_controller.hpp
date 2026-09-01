#pragma once

#include "../model/edit_history.hpp"
#include "../model/view_tree_model.hpp"
#include "../patcher/patch_builder.hpp"
#include <functional>
#include <set>
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
	using refresh_fn_t = std::function<void()>;
	using record_removal_fn_t = std::function<void(const record_removal_record_t &)>;

	merge_controller_t(
	    plugin_session_t & session,
	    record_view_t & record_view,
	    nav_tree_view_t & nav_view,
	    settings_store_t & settings,
	    log_fn_t log_fn);

	void set_refresh_callback(refresh_fn_t refresh_fn);
	void set_record_removal_callback(record_removal_fn_t removal_fn);

	bool create_merged_patch();
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

	bool remove_record_from_plugin(int plugin_idx, const std::string & rec_type, const std::string & record_id);

	void save_merged_patch();

	bool save_plugin(int plugin_idx);
	void save_all_dirty();

private:
	int create_merge_records();
	std::string resolve_merge_output_path() const;
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
	std::set<int> collect_contributing_plugins() const;
	std::vector<patch_builder_t::master_entry_t> build_master_list(const std::set<int> & contributing) const;

	plugin_session_t & m_session;
	record_view_t & m_record_view;
	nav_tree_view_t & m_nav_view;
	settings_store_t & m_settings;
	log_fn_t m_log;
	refresh_fn_t m_refresh;
	record_removal_fn_t m_record_removal;
};
