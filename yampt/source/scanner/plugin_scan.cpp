#include "plugin_scan.hpp"
#include "conflict_compute.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../decoder/sub_record_schema.hpp"
#include "../decoder/view_tree_format.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <map>
#include <set>

static constexpr float tes3_header_version = 1.3f;
static constexpr size_t tes3_hedr_size = 300;
static constexpr size_t tes3_author_offset = 8;
static constexpr size_t tes3_author_max_len = 32;
static constexpr size_t tes3_desc_offset = 40;
static constexpr size_t tes3_desc_max_len = 256;
static constexpr size_t tes3_numrec_offset = 296;
static constexpr size_t tes3_flags_offset = 4;
static constexpr size_t data_sub_record_size = 8;

void plugin_scan_t::load_plugin(const std::string & path)
{
	auto p = std::make_unique<loaded_plugin_t>(path);
	plugins_.push_back(std::move(p));
}

void plugin_scan_t::set_merge_plugin(const std::string & filename)
{
	merge_plugin_idx_ = static_cast<int>(plugins_.size());

	auto p = std::make_unique<loaded_plugin_t>();
	p->path = filename;
	plugins_.push_back(std::move(p));

	merge_records_.clear();
}

void plugin_scan_t::insert_or_update_version(version_descriptor_t desc)
{
	std::string key = desc.rec_type + std::string(1, '\0') + desc.record_id;
	auto it = entry_lookup_.find(key);

	if (it == entry_lookup_.end())
	{
		conflict_entry_t entry;
		entry.rec_type = std::move(desc.rec_type);
		entry.record_id = std::move(desc.record_id);
		entry.display_name = std::move(desc.display_name);
		entry.dial_name = std::move(desc.dial_name);
		entry.conflict_all = conflict_all_t::only_one;

		desc.version.status = conflict_this_t::master;
		entry.versions.push_back(desc.version);

		entry_lookup_[key] = entries_.size();
		entries_.push_back(std::move(entry));
		return;
	}

	auto & entry = entries_[it->second];
	desc.version.status = conflict_this_t::unknown;
	entry.versions.push_back(desc.version);

	if (entry.display_name.empty() && !desc.display_name.empty())
		entry.display_name = std::move(desc.display_name);
}

void plugin_scan_t::rebuild_conflicts()
{
	entries_.clear();
	entry_lookup_.clear();

	for (int pi = 0; pi < static_cast<int>(plugins_.size()); ++pi)
	{
		if (pi == merge_plugin_idx_)
			continue;

		const auto & idx = plugins_[pi]->index;
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

	if (merge_plugin_idx_ >= 0)
	{
		for (size_t mi = 0; mi < merge_records_.size(); ++mi)
		{
			const auto & mr = merge_records_[mi];
			record_version_t ver;
			ver.plugin_idx = merge_plugin_idx_;
			ver.record_index = mi;
			insert_or_update_version({ mr.rec_type, mr.record_id, "", "", ver });
		}
	}

	for (auto & entry : entries_)
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

		if (ver.plugin_idx == merge_plugin_idx_)
			contents[i] = merge_records_[ver.record_index].content;
		else
		{
			plugins_[ver.plugin_idx]->esm.select_record(ver.record_index);
			contents[i] = plugins_[ver.plugin_idx]->esm.get_record().content;

			const auto & plugin_entries = plugins_[ver.plugin_idx]->index.entries();
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
	return plugins_.size();
}

const std::string & plugin_scan_t::plugin_path(int idx) const
{
	return plugins_[idx]->path;
}

std::string plugin_scan_t::plugin_filename(int idx) const
{
	return std::filesystem::path(plugins_[idx]->path).filename().string();
}

const esm_reader_t & plugin_scan_t::plugin(int idx) const
{
	return plugins_[idx]->esm;
}

const plugin_index_t & plugin_scan_t::index(int idx) const
{
	return plugins_[idx]->index;
}

bool plugin_scan_t::is_merge_plugin(int idx) const
{
	return idx == merge_plugin_idx_;
}

const std::vector<conflict_entry_t> & plugin_scan_t::entries() const
{
	return entries_;
}

const conflict_entry_t * plugin_scan_t::find(const std::string & type, const std::string & id) const
{
	std::string key = type + std::string(1, '\0') + id;
	auto it = entry_lookup_.find(key);
	if (it == entry_lookup_.end())
		return nullptr;

	return &entries_[it->second];
}

std::vector<std::string> plugin_scan_t::all_types() const
{
	std::set<std::string> unique;
	for (const auto & entry : entries_)
		unique.insert(entry.rec_type);

	return std::vector<std::string>(unique.begin(), unique.end());
}

void plugin_scan_t::copy_record_to_merge(int source_plugin, size_t record_index)
{
	if (merge_plugin_idx_ < 0)
		return;

	plugins_[source_plugin]->esm.select_record(record_index);
	const auto & rec = plugins_[source_plugin]->esm.get_record();

	const auto & plugin_entries = plugins_[source_plugin]->index.entries();
	if (record_index >= plugin_entries.size())
		return;

	const auto & indexed = plugin_entries[record_index];

	merge_record_t mr;
	mr.rec_type = indexed.rec_type;
	mr.record_id = indexed.record_id;
	mr.content = rec.content;

	for (auto & existing : merge_records_)
	{
		if (existing.rec_type == mr.rec_type && existing.record_id == mr.record_id)
		{
			existing.content = mr.content;
			return;
		}
	}

	merge_records_.push_back(std::move(mr));
}

void plugin_scan_t::remove_from_merge(const std::string & type, const std::string & id)
{
	auto it = std::remove_if(
	    merge_records_.begin(),
	    merge_records_.end(),
	    [&](const merge_record_t & mr) { return mr.rec_type == type && mr.record_id == id; });
	merge_records_.erase(it, merge_records_.end());
}

bool plugin_scan_t::save_merge(
    const std::string & output_path,
    const std::string & author,
    const std::string & description)
{
	if (merge_plugin_idx_ < 0)
		return false;

	std::string header_content = build_tes3_header(author, description);

	std::vector<tools_t::record_t> records;
	records.push_back({ "TES3", header_content, header_content.size(), false });

	for (const auto & mr : merge_records_)
		records.push_back({ mr.rec_type, mr.content, mr.content.size(), false });

	tools_t::write_file(records, output_path);
	return true;
}

bool plugin_scan_t::has_merge() const
{
	return merge_plugin_idx_ >= 0;
}

size_t plugin_scan_t::merge_record_count() const
{
	return merge_records_.size();
}

const std::string & plugin_scan_t::merge_record_content(size_t index) const
{
	return merge_records_[index].content;
}

void plugin_scan_t::copy_record_to_merge_raw(
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & content)
{
	if (merge_plugin_idx_ < 0)
		return;

	for (auto & existing : merge_records_)
	{
		if (existing.rec_type == rec_type && existing.record_id == record_id)
		{
			existing.content = content;
			return;
		}
	}

	merge_record_t mr;
	mr.rec_type = rec_type;
	mr.record_id = record_id;
	mr.content = content;
	merge_records_.push_back(std::move(mr));
}

struct list_item_t
{
	std::string ident;
	uint16_t level;
};

static std::vector<list_item_t> extract_list_items(const std::string & content)
{
	std::vector<list_item_t> items;
	sub_record_iter_t iter(content);
	sub_record_view_t sub;
	std::string current_id;

	while (iter.next(sub))
	{
		if (sub.type == "INAM" || sub.type == "CNAM")
		{
			current_id = std::string(sub.data, sub.size);
			current_id = tools_t::erase_null_chars(current_id);
			continue;
		}

		if (sub.type == "INTV" && !current_id.empty())
		{
			uint16_t level = 0;
			if (sub.size >= 2)
				std::memcpy(&level, sub.data, 2);

			items.push_back({ current_id, level });
			current_id.clear();
		}
	}

	return items;
}

static std::string extract_list_header(const std::string & content)
{
	std::string header_part;
	sub_record_iter_t iter(content);
	sub_record_view_t sub;

	while (iter.next(sub))
	{
		if (sub.type == "INAM" || sub.type == "CNAM" || sub.type == "INTV")
			break;

		header_part += sub.type;
		header_part += tools_t::convert_uint_to_string_byte_array(sub.size);
		header_part += std::string(sub.data, sub.size);
	}

	return header_part;
}

static std::string build_merged_list_record(
    const std::string & rec_type,
    const std::string & header_part,
    const std::vector<list_item_t> & merged_items)
{
	std::string indx_sub = "INDX";
	uint32_t item_count = static_cast<uint32_t>(merged_items.size());
	indx_sub += tools_t::convert_uint_to_string_byte_array(4);
	indx_sub += std::string(reinterpret_cast<const char *>(&item_count), 4);

	const std::string & item_sub_type = (rec_type == "LEVI") ? "INAM" : "CNAM";
	std::string items_part;
	for (const auto & item : merged_items)
	{
		std::string id_data = item.ident;
		id_data.push_back('\0');

		items_part += item_sub_type;
		items_part += tools_t::convert_uint_to_string_byte_array(id_data.size());
		items_part += id_data;

		items_part += "INTV";
		items_part += tools_t::convert_uint_to_string_byte_array(2);
		items_part += std::string(reinterpret_cast<const char *>(&item.level), 2);
	}

	std::string body = header_part + indx_sub + items_part;

	std::string record;
	record += rec_type;
	record += tools_t::convert_uint_to_string_byte_array(body.size());
	record += std::string(8, '\0');
	record += body;

	return record;
}

void plugin_scan_t::merge_leveled_list(const conflict_entry_t & entry)
{
	if (entry.versions.size() < 2)
		return;

	auto get_content = [&](const record_version_t & ver) -> std::string
	{
		if (ver.plugin_idx == merge_plugin_idx_)
			return merge_records_[ver.record_index].content;

		plugins_[ver.plugin_idx]->esm.select_record(ver.record_index);
		return plugins_[ver.plugin_idx]->esm.get_record().content;
	};

	const std::string & master_content = get_content(entry.versions[0]);
	auto master_items = extract_list_items(master_content);

	std::set<std::string> merged_keys;
	for (const auto & item : master_items)
		merged_keys.insert(item.ident + "\x00" + std::to_string(item.level));

	std::vector<list_item_t> merged = master_items;

	for (size_t vi = 1; vi < entry.versions.size(); ++vi)
	{
		if (entry.versions[vi].plugin_idx == merge_plugin_idx_)
			continue;

		const std::string & ver_content = get_content(entry.versions[vi]);
		auto ver_items = extract_list_items(ver_content);

		for (const auto & item : ver_items)
		{
			std::string key = item.ident + "\x00" + std::to_string(item.level);
			if (merged_keys.count(key))
				continue;

			merged.push_back(item);
			merged_keys.insert(key);
		}
	}

	const auto & header_part = extract_list_header(master_content);
	const auto & record = build_merged_list_record(entry.rec_type, header_part, merged);
	copy_record_to_merge_raw(entry.rec_type, entry.record_id, record);
}

void plugin_scan_t::merge_dialogue(const conflict_entry_t & entry)
{
	if (entry.rec_type != "DIAL")
		return;

	auto get_content = [&](const record_version_t & ver) -> std::string
	{
		if (ver.plugin_idx == merge_plugin_idx_)
			return merge_records_[ver.record_index].content;

		plugins_[ver.plugin_idx]->esm.select_record(ver.record_index);
		return plugins_[ver.plugin_idx]->esm.get_record().content;
	};

	std::string winning_dial = get_content(entry.versions.back());
	copy_record_to_merge_raw("DIAL", entry.record_id, winning_dial);

	std::vector<std::string> merged_info_ids;
	std::map<std::string, std::string> info_contents;

	for (size_t vi = 0; vi < entry.versions.size(); ++vi)
	{
		const auto & ver = entry.versions[vi];
		if (ver.plugin_idx == merge_plugin_idx_)
			continue;

		int pi = ver.plugin_idx;
		size_t dial_rec_idx = ver.record_index;

		const auto & plugin_entries = plugins_[pi]->index.entries();
		for (size_t ei = dial_rec_idx + 1; ei < plugin_entries.size(); ++ei)
		{
			if (plugin_entries[ei].rec_type != "INFO")
				break;

			if (plugin_entries[ei].dial_name != entry.record_id)
				break;

			const auto & info_id = plugin_entries[ei].record_id;

			plugins_[pi]->esm.select_record(plugin_entries[ei].record_index);
			std::string content = plugins_[pi]->esm.get_record().content;

			if (info_contents.find(info_id) == info_contents.end())
				merged_info_ids.push_back(info_id);

			info_contents[info_id] = content;
		}
	}

	for (const auto & info_id : merged_info_ids)
	{
		const auto & content = info_contents[info_id];
		copy_record_to_merge_raw("INFO", info_id, content);
	}
}

size_t plugin_scan_t::itm_count(int plugin_idx) const
{
	size_t count = 0;
	for (const auto & entry : entries_)
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
	for (const auto & entry : entries_)
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

static std::string build_hedr_data(size_t record_count, const std::string & author, const std::string & description)
{
	std::string hedr_data(tes3_hedr_size, '\0');

	float version = tes3_header_version;
	std::memcpy(&hedr_data[0], &version, 4);

	uint32_t flags = 0;
	std::memcpy(&hedr_data[tes3_flags_offset], &flags, 4);

	for (size_t i = 0; i < author.size() && i < tes3_author_max_len; ++i)
		hedr_data[tes3_author_offset + i] = author[i];

	for (size_t i = 0; i < description.size() && i < tes3_desc_max_len; ++i)
		hedr_data[tes3_desc_offset + i] = description[i];

	uint32_t num_records = static_cast<uint32_t>(record_count);
	std::memcpy(&hedr_data[tes3_numrec_offset], &num_records, 4);

	return hedr_data;
}

std::string plugin_scan_t::build_tes3_header(const std::string & author, const std::string & description)
{
	const auto & hedr_data = build_hedr_data(merge_records_.size(), author, description);

	std::string body;
	body += "HEDR";
	body += tools_t::convert_uint_to_string_byte_array(tes3_hedr_size);
	body += hedr_data;

	for (int i = 0; i < static_cast<int>(plugins_.size()); ++i)
	{
		if (i == merge_plugin_idx_)
			continue;

		std::string filename = plugin_filename(i);
		filename.push_back('\0');

		body += "MAST";
		body += tools_t::convert_uint_to_string_byte_array(filename.size());
		body += filename;

		uint64_t file_size = 0;
		try
		{
			file_size = std::filesystem::file_size(plugins_[i]->path);
		}
		catch (...)
		{}

		std::string size_data(data_sub_record_size, '\0');
		std::memcpy(&size_data[0], &file_size, data_sub_record_size);

		body += "DATA";
		body += tools_t::convert_uint_to_string_byte_array(data_sub_record_size);
		body += size_data;
	}

	std::string header;
	header += "TES3";
	header += tools_t::convert_uint_to_string_byte_array(body.size());

	std::string unknown(4, '\0');
	header += unknown;

	std::string rec_flags(4, '\0');
	header += rec_flags;

	header += body;

	return header;
}
