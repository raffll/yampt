#include "plugin_scan.hpp"
#include "conflict_compute.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../decoder/sub_record_schema.hpp"
#include "../decoder/view_tree_format.hpp"
#include <algorithm>
#include <cstring>
#include <set>

void plugin_scan_t::load_plugin(const std::string & path)
{
	auto p = std::make_unique<loaded_plugin_t>(path);
	m_plugins.push_back(std::move(p));
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
			if (rec.rec_type == "TES3")
				continue;

			record_version_t ver;
			ver.plugin_idx = pi;
			ver.record_index = rec.record_index;
			insert_or_update_version({ rec.rec_type, rec.record_id, rec.display_name, rec.dial_name, ver });
		}
	}

	if (m_merge_plugin_idx >= 0)
	{
		for (size_t mi = 0; mi < m_merge_records.size(); ++mi)
		{
			const auto & mr = m_merge_records[mi];
			record_version_t ver;
			ver.plugin_idx = m_merge_plugin_idx;
			ver.record_index = mi;
			insert_or_update_version({ mr.rec_type, mr.record_id, "", "", ver });
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

	void accumulate(const std::vector<std::string> & values)
	{
		const auto level = compute_conflict_all(values);
		if (level > worst_all)
			worst_all = level;

		per_slot_this.push_back(compute_conflict_this(values));
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
				continue;

			const auto & sv = ctx.sr.parsed[vi][ctx.slot.indices[vi]];
			if (fdef.offset >= sv.size)
				continue;

			uint32_t val = 0;
			std::memcpy(&val, sv.data + fdef.offset, std::min(bytes, sv.size - fdef.offset));
			bit_values[vi] = (val & (1u << bit)) ? "1" : "0";
		}

		ctx.accum.accumulate(bit_values);
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
				continue;

			const auto & sv = ctx.sr.parsed[vi][ctx.slot.indices[vi]];
			field_values[vi] = decode_field(fdef, sv.data, sv.size);
		}

		ctx.accum.accumulate(field_values);
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
	{
		if (is_deleted[i])
		{
			entry.versions[i].status = conflict_this_t::deleted;
			continue;
		}

		entry.versions[i].status = worst_this[i];
	}
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
			contents[i] = m_merge_records[ver.record_index].content;
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
	    std::make_unique<slot_result_t>(build_conflict_slots(entry.rec_type, std::move(contents), is_deleted));

	conflict_accumulator_t accum;
	const auto & sr = *entry.slot_result;

	for (const auto & slot : sr.aligned)
	{
		std::vector<std::string> slot_values(ver_count);
		const char * first_data = nullptr;
		size_t first_size = 0;

		for (size_t vi = 0; vi < ver_count; ++vi)
		{
			if (is_deleted[vi] || slot.indices[vi] == SIZE_MAX)
				continue;

			const auto & sv = sr.parsed[vi][slot.indices[vi]];
			slot_values[vi] = format_value(sv.data, sv.size);
			if (!first_data)
			{
				first_data = sv.data;
				first_size = sv.size;
			}
		}

		const auto * schema = find_schema(entry.rec_type, slot.key.type, first_size);
		slot_eval_context_t ctx { slot, sr, is_deleted, accum };
		if (schema && schema->field_count > 0 && first_data)
			evaluate_schema_fields(schema, ctx);
		else
			accum.accumulate(slot_values);
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

