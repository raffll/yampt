#include "plugin_scan.hpp"
#include "../utility/string_utils.hpp"
#include "../utility/includes.hpp"
#include "../utility/app_logger.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../decoder/sub_record_schema.hpp"
#include "../decoder/view_tree_format.hpp"
#include "record_conflict.hpp"
#include <algorithm>
#include <cstring>
#include <set>

void plugin_scan_t::load_plugin(const std::string & path)
{
	auto p = std::make_unique<loaded_plugin_t>(path);
	m_plugins.push_back(std::move(p));
}

void plugin_scan_t::loaded_plugin_t::parse_master_list()
{
	if (!esm.is_loaded() || esm.get_records().empty())
		return;

	esm.select_record(0);
	const auto & content = esm.get_record().content;
	sub_record_iter_t iter(content);
	sub_record_view_t sub;

	while (iter.next(sub))
	{
		if (sub.type != "MAST")
			continue;

		std::string master_name(sub.data, sub.size);
		master_name = string_utils::erase_null_chars(master_name);
		master_files.push_back(std::move(master_name));
	}
}

const std::vector<std::string> & plugin_scan_t::master_list(int idx) const
{
	return m_plugins[idx]->master_files;
}

uint64_t plugin_scan_t::resolve_frmr(int plugin_idx, uint32_t raw_frmr) const
{
	const uint32_t local_master = (raw_frmr & 0xFF000000) >> 24;
	const uint32_t ref_index = raw_frmr & 0x00FFFFFF;

	if (local_master == 0)
		return (static_cast<uint64_t>(plugin_idx) << 32) | ref_index;

	const auto & masters = m_plugins[plugin_idx]->master_files;
	const size_t master_array_idx = local_master - 1;
	if (master_array_idx >= masters.size())
		return (static_cast<uint64_t>(plugin_idx) << 32) | ref_index;

	const auto & master_filename = masters[master_array_idx];
	for (int i = 0; i < static_cast<int>(m_plugins.size()); ++i)
	{
		if (plugin_filename(i) == master_filename)
			return (static_cast<uint64_t>(i) << 32) | ref_index;
	}

	return (static_cast<uint64_t>(plugin_idx) << 32) | ref_index;
}

void plugin_scan_t::insert_or_update_version(version_descriptor_t desc)
{
	std::string key = desc.rec_type + std::string(1, '\0') + desc.record_id;
	auto it = m_entry_lookup.find(key);

	if (it == m_entry_lookup.end())
	{
		conflict_entry_t entry;
		entry.rec_type = std::move(desc.rec_type);
		entry.record_id = std::move(desc.record_id);
		entry.display_name = std::move(desc.display_name);
		entry.dial_name = std::move(desc.dial_name);
		entry.conflict_all = conflict_all_t::only_one;

		desc.version.status = conflict_this_t::master;
		entry.versions.push_back(desc.version);

		m_entry_lookup[key] = m_entries.size();
		m_entries.push_back(std::move(entry));
		return;
	}

	auto & entry = m_entries[it->second];
	desc.version.status = conflict_this_t::unknown;
	entry.versions.push_back(desc.version);

	if (entry.display_name.empty() && !desc.display_name.empty())
		entry.display_name = std::move(desc.display_name);
}

void plugin_scan_t::rebuild_conflicts()
{
	m_entries.clear();
	m_entry_lookup.clear();

	for (int pi = 0; pi < static_cast<int>(m_plugins.size()); ++pi)
	{
		if (pi == m_merge_plugin_idx)
			continue;

		const auto & idx = m_plugins[pi]->index;
		for (const auto & rec : idx.entries())
		{
			record_version_t ver;
			ver.plugin_idx = pi;
			ver.record_index = rec.record_index;

			auto record_id = rec.record_id;
			if (rec.rec_type == "TES3")
				record_id = plugin_filename(pi);

			insert_or_update_version({ rec.rec_type, record_id, rec.display_name, rec.dial_name, ver });

			if (rec.has_dele)
			{
				const std::string lookup_key = rec.rec_type + std::string(1, '\0') + record_id;
				auto it_entry = m_entry_lookup.find(lookup_key);
				if (it_entry != m_entry_lookup.end())
					m_entries[it_entry->second].has_dele = true;
			}
		}
	}

	if (m_merge_plugin_idx >= 0)
	{
		const auto & store_records = m_merge_store.records();
		for (size_t mi = 0; mi < store_records.size(); ++mi)
		{
			const auto & merge_rec = store_records[mi];
			record_version_t ver;
			ver.plugin_idx = m_merge_plugin_idx;
			ver.record_index = mi;
			insert_or_update_version({ merge_rec.rec_type, merge_rec.record_id, "", "", ver });
		}
	}

	for (auto & entry : m_entries)
	{
		if (entry.versions.size() >= 2)
			compute_conflict(entry);
	}
}

struct conflict_accumulator_t
{
	conflict_all_t worst_all = conflict_all_t::only_one;
	std::vector<std::vector<conflict_this_t>> per_slot_this;

	void accumulate(const std::vector<std::string> & values, bool skip_non_existent = false)
	{
		const auto level = skip_non_existent ? record_conflict_t::compute_conflict_all_skip_empty(values) : record_conflict_t::compute_conflict_all(values);

		if (level > worst_all)
			worst_all = level;

		per_slot_this.push_back(
		    skip_non_existent ? record_conflict_t::compute_conflict_this_skip_empty(values) : record_conflict_t::compute_conflict_this(values));
	}
};

static size_t flag_byte_width(field_type_t field_type)
{
	if (field_type == field_type_t::flags_u8)
		return 1;

	if (field_type == field_type_t::flags_u16)
		return 2;

	return 4;
}

struct slot_eval_context_t
{
	const aligned_slot_t & slot;
	const slot_result_t & sr;
	const std::vector<bool> & is_deleted;
	conflict_accumulator_t & accum;
	bool skip_non_existent = false;
};

static void evaluate_flag_bits(const field_def_t & fdef, const slot_eval_context_t & ctx)
{
	const size_t ver_count = ctx.is_deleted.size();
	const size_t bytes = flag_byte_width(fdef.type);

	for (int bit = 0; bit < fdef.flag_count; ++bit)
	{
		if (fdef.flag_names[bit][0] == '_')
			continue;

		std::vector<std::string> bit_values(ver_count);
		for (size_t vi = 0; vi < ver_count; ++vi)
		{
			if (ctx.is_deleted[vi] || ctx.slot.indices[vi] == SIZE_MAX)
			{
				if (ctx.skip_non_existent)
					bit_values[vi] = non_existent_value;

				continue;
			}

			const auto & sv = ctx.sr.parsed[vi][ctx.slot.indices[vi]];
			if (fdef.offset >= sv.size)
				continue;

			uint32_t val = 0;
			std::memcpy(&val, sv.data + fdef.offset, std::min(bytes, sv.size - fdef.offset));
			bit_values[vi] = (val & (1u << bit)) ? "1" : "0";
		}

		ctx.accum.accumulate(bit_values, ctx.skip_non_existent);
	}
}

static void evaluate_schema_fields(const sub_record_schema_t * schema, const slot_eval_context_t & ctx)
{
	const size_t ver_count = ctx.is_deleted.size();

	for (size_t fi = 0; fi < schema->field_count; ++fi)
	{
		const auto & fdef = schema->fields[fi];

		const bool is_flags =
		    (fdef.type == field_type_t::flags_u8 || fdef.type == field_type_t::flags_u16 ||
		     fdef.type == field_type_t::flags_u32);

		if (is_flags && fdef.flag_names && fdef.flag_count > 0)
		{
			evaluate_flag_bits(fdef, ctx);
			continue;
		}

		std::vector<std::string> field_values(ver_count);
		for (size_t vi = 0; vi < ver_count; ++vi)
		{
			if (ctx.is_deleted[vi] || ctx.slot.indices[vi] == SIZE_MAX)
			{
				if (ctx.skip_non_existent)
					field_values[vi] = non_existent_value;

				continue;
			}

			const auto & sv = ctx.sr.parsed[vi][ctx.slot.indices[vi]];
			field_values[vi] = decode_field(fdef, sv.data, sv.size);
		}

		ctx.accum.accumulate(field_values, ctx.skip_non_existent);
	}
}

static void apply_worst_this(
    conflict_entry_t & entry,
    const conflict_accumulator_t & accum,
    const std::vector<bool> & is_deleted)
{
	const size_t ver_count = entry.versions.size();
	std::vector<conflict_this_t> worst_this(ver_count, conflict_this_t::unknown);
	worst_this[0] = conflict_this_t::master;

	for (const auto & slot_ct : accum.per_slot_this)
	{
		for (size_t i = 1; i < ver_count; ++i)
		{
			if (slot_ct[i] > worst_this[i])
				worst_this[i] = slot_ct[i];
		}
	}

	for (size_t i = 1; i < ver_count; ++i)
		entry.versions[i].status = worst_this[i];
}

void plugin_scan_t::compute_conflict(conflict_entry_t & entry)
{
	const size_t ver_count = entry.versions.size();

	std::vector<std::string> contents(ver_count);
	std::vector<bool> is_deleted(ver_count, false);

	for (size_t i = 0; i < ver_count; ++i)
	{
		const auto & ver = entry.versions[i];

		if (ver.plugin_idx == m_merge_plugin_idx)
			contents[i] = m_merge_store.record_content(ver.record_index);
		else
		{
			m_plugins[ver.plugin_idx]->esm.select_record(ver.record_index);
			contents[i] = m_plugins[ver.plugin_idx]->esm.get_record().content;

			const auto & plugin_entries = m_plugins[ver.plugin_idx]->index.entries();
			if (ver.record_index < plugin_entries.size() && plugin_entries[ver.record_index].has_dele)
				is_deleted[i] = true;
		}
	}

	entry.slot_result =
	    std::make_unique<slot_result_t>(conflict_slot_builder_t::build(entry.rec_type, std::move(contents), is_deleted));

	conflict_accumulator_t accum;
	const auto & sr = *entry.slot_result;

	for (const auto & slot : sr.aligned)
	{
		const auto policy = record_conflict_t::find_conflict_policy(entry.rec_type, slot.key.type);
		if (policy.ignore_conflict)
			continue;

		std::vector<std::string> slot_values(ver_count);
		const char * first_data = nullptr;
		size_t first_size = 0;

		for (size_t vi = 0; vi < ver_count; ++vi)
		{
			if (is_deleted[vi] || slot.indices[vi] == SIZE_MAX)
			{
				if (policy.skip_non_existent)
					slot_values[vi] = non_existent_value;

				continue;
			}

			const auto & sv = sr.parsed[vi][slot.indices[vi]];
			slot_values[vi] = format_value(sv.data, sv.size);
			if (!first_data)
			{
				first_data = sv.data;
				first_size = sv.size;
			}
		}

		const auto * schema = find_schema(entry.rec_type, slot.key.type, first_size);
		slot_eval_context_t ctx { slot, sr, is_deleted, accum, policy.skip_non_existent };
		if (schema && schema->field_count > 0 && first_data)
			evaluate_schema_fields(schema, ctx);
		else
			accum.accumulate(slot_values, policy.skip_non_existent);
	}

	entry.conflict_all = accum.worst_all;

	if (accum.worst_all <= conflict_all_t::only_one)
		return;

	apply_worst_this(entry, accum, is_deleted);
}

size_t plugin_scan_t::plugin_count() const
{
	return m_plugins.size();
}

const std::string & plugin_scan_t::plugin_path(int idx) const
{
	return m_plugins[idx]->path;
}

std::string plugin_scan_t::plugin_filename(int idx) const
{
	return std::filesystem::path(m_plugins[idx]->path).filename().string();
}

const esm_reader_t & plugin_scan_t::plugin(int idx) const
{
	return m_plugins[idx]->esm;
}

const plugin_index_t & plugin_scan_t::index(int idx) const
{
	return m_plugins[idx]->index;
}

bool plugin_scan_t::is_merge_plugin(int idx) const
{
	return idx == m_merge_plugin_idx;
}

const std::vector<conflict_entry_t> & plugin_scan_t::entries() const
{
	return m_entries;
}

const conflict_entry_t * plugin_scan_t::find(const std::string & type, const std::string & id) const
{
	std::string key = type + std::string(1, '\0') + id;
	auto it = m_entry_lookup.find(key);
	if (it == m_entry_lookup.end())
		return nullptr;

	return &m_entries[it->second];
}

std::vector<std::string> plugin_scan_t::all_types() const
{
	std::set<std::string> unique;
	for (const auto & entry : m_entries)
		unique.insert(entry.rec_type);

	return std::vector<std::string>(unique.begin(), unique.end());
}

void plugin_scan_t::set_merge_plugin(const std::string & filename)
{
	m_merge_plugin_idx = static_cast<int>(m_plugins.size());

	auto plugin = std::make_unique<loaded_plugin_t>();
	plugin->path = filename;
	m_plugins.push_back(std::move(plugin));

	m_merge_store.clear();
}

void plugin_scan_t::set_merge_plugin_from_loaded(int plugin_idx)
{
	if (plugin_idx < 0 || plugin_idx >= static_cast<int>(m_plugins.size()))
		return;

	m_merge_plugin_idx = plugin_idx;
	m_merge_store.clear();

	auto & plugin = *m_plugins[plugin_idx];
	const auto & entries = plugin.index.entries();

	for (size_t i = 0; i < entries.size(); ++i)
	{
		plugin.esm.select_record(entries[i].record_index);
		const auto & rec = plugin.esm.get_record();
		m_merge_store.add(entries[i].rec_type, entries[i].record_id, rec.content);
	}
}

void plugin_scan_t::clear_merge_records()
{
	m_merge_store.clear();
}

std::vector<merge_record_t> plugin_scan_t::collect_pinned_records() const
{
	return m_merge_store.collect_pinned();
}

void plugin_scan_t::restore_pinned_records(const std::vector<merge_record_t> & pinned)
{
	m_merge_store.restore_pinned(pinned);
}

void plugin_scan_t::copy_record_to_merge(int source_plugin, size_t record_index)
{
	if (m_merge_plugin_idx < 0)
		return;

	m_plugins[source_plugin]->esm.select_record(record_index);
	const auto & rec = m_plugins[source_plugin]->esm.get_record();

	const auto & plugin_entries = m_plugins[source_plugin]->index.entries();
	if (record_index >= plugin_entries.size())
		return;

	const auto & indexed = plugin_entries[record_index];
	copy_record_to_merge_raw(indexed.rec_type, indexed.record_id, rec.content);
}

void plugin_scan_t::copy_record_to_merge_raw(
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & content)
{
	if (m_merge_plugin_idx < 0)
		return;

	m_merge_store.update_or_add(rec_type, record_id, content);
}

void plugin_scan_t::pin_record_to_merge(
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & content)
{
	if (m_merge_plugin_idx < 0)
		return;

	m_merge_store.update_or_add_pinned(rec_type, record_id, content);
}

bool plugin_scan_t::is_merge_pinned(const std::string & rec_type, const std::string & record_id) const
{
	return m_merge_store.is_pinned(rec_type, record_id);
}

const std::string * plugin_scan_t::find_merge_content(const std::string & rec_type, const std::string & record_id) const
{
	return m_merge_store.find_content(rec_type, record_id);
}

void plugin_scan_t::remove_from_merge(const std::string & type, const std::string & id)
{
	m_merge_store.remove(type, id);
}

void plugin_scan_t::recompute_single_conflict(const std::string & rec_type, const std::string & record_id)
{
	const std::string lookup_key = rec_type + std::string(1, '\0') + record_id;
	auto it_entry = m_entry_lookup.find(lookup_key);
	if (it_entry == m_entry_lookup.end())
	{
		const auto & store_records = m_merge_store.records();
		for (size_t mi = 0; mi < store_records.size(); ++mi)
		{
			const auto & merge_rec = store_records[mi];
			if (merge_rec.rec_type != rec_type || merge_rec.record_id != record_id)
				continue;

			record_version_t ver;
			ver.plugin_idx = m_merge_plugin_idx;
			ver.record_index = mi;
			insert_or_update_version({ rec_type, record_id, "", "", ver });
			break;
		}

		return;
	}

	auto & entry = m_entries[it_entry->second];

	entry.versions.erase(
	    std::remove_if(
	        entry.versions.begin(),
	        entry.versions.end(),
	        [this](const record_version_t & ver) { return ver.plugin_idx == m_merge_plugin_idx; }),
	    entry.versions.end());

	const auto & store_records = m_merge_store.records();
	for (size_t mi = 0; mi < store_records.size(); ++mi)
	{
		const auto & merge_rec = store_records[mi];
		if (merge_rec.rec_type != rec_type || merge_rec.record_id != record_id)
			continue;

		record_version_t ver;
		ver.plugin_idx = m_merge_plugin_idx;
		ver.record_index = mi;
		entry.versions.push_back(ver);
		break;
	}

	entry.conflict_all = conflict_all_t::only_one;
	entry.slot_result.reset();

	if (entry.versions.size() >= 2)
		compute_conflict(entry);
}

std::string plugin_scan_t::read_record_content(int plugin_idx, size_t record_index)
{
	if (plugin_idx == m_merge_plugin_idx)
	{
		if (record_index < m_merge_store.count())
			return m_merge_store.record_content(record_index);

		return {};
	}

	if (plugin_idx < 0 || plugin_idx >= static_cast<int>(m_plugins.size()))
		return {};

	m_plugins[plugin_idx]->esm.select_record(record_index);
	return m_plugins[plugin_idx]->esm.get_record().content;
}

bool plugin_scan_t::has_merge() const
{
	return m_merge_plugin_idx >= 0;
}

size_t plugin_scan_t::merge_record_count() const
{
	return m_merge_store.count();
}

const std::string & plugin_scan_t::merge_record_content(size_t index) const
{
	return m_merge_store.record_content(index);
}

const std::string & plugin_scan_t::merge_record_type(size_t index) const
{
	return m_merge_store.record_type(index);
}

const std::string & plugin_scan_t::merge_record_id(size_t index) const
{
	return m_merge_store.record_id(index);
}

size_t plugin_scan_t::itm_count(int plugin_idx) const
{
	size_t count = 0;
	for (const auto & entry : m_entries)
	{
		for (const auto & ver : entry.versions)
		{
			if (ver.plugin_idx == plugin_idx && ver.status == conflict_this_t::identical_to_master)
			{
				++count;
				break;
			}
		}
	}
	return count;
}

std::vector<const conflict_entry_t *> plugin_scan_t::itm_entries(int plugin_idx) const
{
	std::vector<const conflict_entry_t *> result;
	for (const auto & entry : m_entries)
	{
		for (const auto & ver : entry.versions)
		{
			if (ver.plugin_idx == plugin_idx && ver.status == conflict_this_t::identical_to_master)
			{
				result.push_back(&entry);
				break;
			}
		}
	}
	return result;
}
