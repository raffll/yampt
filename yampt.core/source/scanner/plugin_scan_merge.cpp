#include "../decoder/sub_record_iter.hpp"
#include "plugin_scan.hpp"
#include "sub_record_merge.hpp"
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

void plugin_scan_t::set_merge_plugin(const std::string & filename)
{
	m_merge_plugin_idx = static_cast<int>(m_plugins.size());

	auto p = std::make_unique<loaded_plugin_t>();
	p->path = filename;
	m_plugins.push_back(std::move(p));

	m_merge_records.clear();
}

void plugin_scan_t::clear_merge_records()
{
	m_merge_records.clear();
}

std::vector<plugin_scan_t::merge_record_t> plugin_scan_t::collect_pinned_records() const
{
	std::vector<merge_record_t> pinned;
	for (const auto & mr : m_merge_records)
	{
		if (mr.pinned)
			pinned.push_back(mr);
	}
	return pinned;
}

void plugin_scan_t::restore_pinned_records(const std::vector<merge_record_t> & pinned)
{
	for (const auto & pr : pinned)
	{
		bool replaced = false;
		for (auto & existing : m_merge_records)
		{
			if (existing.rec_type == pr.rec_type && existing.record_id == pr.record_id)
			{
				existing.content = pr.content;
				existing.pinned = true;
				replaced = true;
				break;
			}
		}

		if (!replaced)
			m_merge_records.push_back(pr);
	}
}

void plugin_scan_t::set_merge_plugin_from_loaded(int plugin_idx)
{
	if (plugin_idx < 0 || plugin_idx >= static_cast<int>(m_plugins.size()))
		return;

	m_merge_plugin_idx = plugin_idx;
	m_merge_records.clear();

	auto & plugin = *m_plugins[plugin_idx];
	const auto & entries = plugin.index.entries();

	for (size_t i = 0; i < entries.size(); ++i)
	{
		plugin.esm.select_record(entries[i].record_index);
		const auto & rec = plugin.esm.get_record();
		m_merge_records.push_back({ entries[i].rec_type, entries[i].record_id, rec.content });
	}
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

	merge_record_t mr;
	mr.rec_type = indexed.rec_type;
	mr.record_id = indexed.record_id;
	mr.content = rec.content;

	for (auto & existing : m_merge_records)
	{
		if (existing.rec_type == mr.rec_type && existing.record_id == mr.record_id)
		{
			existing.content = mr.content;
			return;
		}
	}

	m_merge_records.push_back(std::move(mr));
}

void plugin_scan_t::copy_record_to_merge_raw(
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & content)
{
	if (m_merge_plugin_idx < 0)
		return;

	for (auto & existing : m_merge_records)
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
	m_merge_records.push_back(std::move(mr));
}

void plugin_scan_t::pin_record_to_merge(
    const std::string & rec_type,
    const std::string & record_id,
    const std::string & content)
{
	if (m_merge_plugin_idx < 0)
		return;

	for (auto & existing : m_merge_records)
	{
		if (existing.rec_type == rec_type && existing.record_id == record_id)
		{
			existing.content = content;
			existing.pinned = true;
			return;
		}
	}

	merge_record_t mr;
	mr.rec_type = rec_type;
	mr.record_id = record_id;
	mr.content = content;
	mr.pinned = true;
	m_merge_records.push_back(std::move(mr));
}

bool plugin_scan_t::is_merge_pinned(const std::string & rec_type, const std::string & record_id) const
{
	for (const auto & mr : m_merge_records)
	{
		if (mr.rec_type == rec_type && mr.record_id == record_id)
			return mr.pinned;
	}
	return false;
}

void plugin_scan_t::remove_from_merge(const std::string & type, const std::string & id)
{
	auto it = std::remove_if(
	    m_merge_records.begin(),
	    m_merge_records.end(),
	    [&](const merge_record_t & mr) { return mr.rec_type == type && mr.record_id == id; });
	m_merge_records.erase(it, m_merge_records.end());
}

const std::string * plugin_scan_t::find_merge_content(const std::string & rec_type, const std::string & record_id) const
{
	for (const auto & mr : m_merge_records)
	{
		if (mr.rec_type == rec_type && mr.record_id == record_id)
			return &mr.content;
	}
	return nullptr;
}

std::string plugin_scan_t::read_record_content(int plugin_idx, size_t record_index)
{
	if (plugin_idx == m_merge_plugin_idx)
	{
		if (record_index < m_merge_records.size())
			return m_merge_records[record_index].content;

		return {};
	}

	if (plugin_idx < 0 || plugin_idx >= static_cast<int>(m_plugins.size()))
		return {};

	m_plugins[plugin_idx]->esm.select_record(record_index);
	return m_plugins[plugin_idx]->esm.get_record().content;
}

bool plugin_scan_t::save_merge(
    const std::string & output_path,
    const std::string & author,
    const std::string & description)
{
	if (m_merge_plugin_idx < 0)
		return false;

	if (m_merge_records.empty())
		return false;

	std::string header_content = build_tes3_header(author, description);

	std::vector<tools_t::record_t> records;
	records.push_back({ "TES3", header_content, header_content.size(), false });

	for (const auto & mr : m_merge_records)
		records.push_back({ mr.rec_type, mr.content, mr.content.size(), false });

	const auto temp_path = output_path + ".tmp";
	tools_t::write_file(records, temp_path);

	if (!std::filesystem::exists(temp_path))
		return false;

	std::error_code error_code;
	std::filesystem::rename(temp_path, output_path, error_code);

	if (!error_code)
		return true;

	std::filesystem::copy_file(temp_path, output_path, std::filesystem::copy_options::overwrite_existing, error_code);

	std::filesystem::remove(temp_path, error_code);

	if (std::filesystem::exists(output_path))
		return true;

	return false;
}

bool plugin_scan_t::has_merge() const
{
	return m_merge_plugin_idx >= 0;
}

size_t plugin_scan_t::merge_record_count() const
{
	return m_merge_records.size();
}

const std::string & plugin_scan_t::merge_record_content(size_t index) const
{
	return m_merge_records[index].content;
}

const std::string & plugin_scan_t::merge_record_type(size_t index) const
{
	return m_merge_records[index].rec_type;
}

const std::string & plugin_scan_t::merge_record_id(size_t index) const
{
	return m_merge_records[index].record_id;
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

		if (sub.type == "INDX")
			continue;

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

using item_count_map_t = std::map<std::pair<std::string, uint16_t>, size_t>;

static item_count_map_t build_item_count_map(const std::vector<list_item_t> & items)
{
	item_count_map_t counts;
	for (const auto & item : items)
		++counts[{ item.ident, item.level }];

	return counts;
}

static bool is_item_deleted(
    const std::pair<std::string, uint16_t> & item_key,
    const item_count_map_t & first_map,
    const std::vector<item_count_map_t> & non_first_maps)
{
	const auto it_first = first_map.find(item_key);
	if (it_first == first_map.end() || it_first->second == 0)
		return false;

	for (const auto & version_map : non_first_maps)
	{
		const auto it_ver = version_map.find(item_key);
		if (it_ver == version_map.end())
			return true;
	}

	return false;
}

static size_t compute_merged_count(
    const std::pair<std::string, uint16_t> & item_key,
    const std::vector<item_count_map_t> & non_first_maps)
{
	size_t max_count = 0;
	for (const auto & version_map : non_first_maps)
	{
		const auto it_ver = version_map.find(item_key);
		if (it_ver != version_map.end() && it_ver->second > max_count)
			max_count = it_ver->second;
	}

	return max_count;
}

static std::vector<list_item_t> build_merged_items(
    const item_count_map_t & first_map,
    const std::vector<item_count_map_t> & non_first_maps)
{
	std::set<std::pair<std::string, uint16_t>> all_keys;
	for (const auto & [key, count] : first_map)
		all_keys.insert(key);

	for (const auto & version_map : non_first_maps)
	{
		for (const auto & [key, count] : version_map)
			all_keys.insert(key);
	}

	std::vector<list_item_t> merged;
	for (const auto & item_key : all_keys)
	{
		if (is_item_deleted(item_key, first_map, non_first_maps))
			continue;

		const auto merged_count = compute_merged_count(item_key, non_first_maps);
		for (size_t i = 0; i < merged_count; ++i)
			merged.push_back({ item_key.first, item_key.second });
	}

	return merged;
}

static void sort_merged_items(std::vector<list_item_t> & items)
{
	std::sort(
	    items.begin(),
	    items.end(),
	    [](const list_item_t & left, const list_item_t & right)
	{
		if (left.level != right.level)
			return left.level < right.level;

		return left.ident < right.ident;
	});
}

void plugin_scan_t::merge_leveled_list(const conflict_entry_t & entry)
{
	if (entry.versions.size() < 2)
		return;

	auto get_content = [&](const record_version_t & ver) -> std::string
	{
		if (ver.plugin_idx == m_merge_plugin_idx)
			return m_merge_records[ver.record_index].content;

		m_plugins[ver.plugin_idx]->esm.select_record(ver.record_index);
		return m_plugins[ver.plugin_idx]->esm.get_record().content;
	};

	std::vector<std::string> version_contents;
	for (const auto & ver : entry.versions)
	{
		if (ver.plugin_idx == m_merge_plugin_idx)
			continue;

		version_contents.push_back(get_content(ver));
	}

	merge_input_t input;
	input.rec_type = entry.rec_type;
	input.record_id = entry.record_id;
	input.version_contents = std::move(version_contents);

	const auto result = leveled_list_merge_t::merge(input);
	if (!result.content.empty())
		copy_record_to_merge_raw(entry.rec_type, entry.record_id, result.content);
}

merge_result_t leveled_list_merge_t::merge(const merge_input_t & input)
{
	const auto & versions = input.version_contents;
	if (versions.size() < 2)
		return { false, {} };

	const auto & first_content = versions.front();
	const auto first_map = build_item_count_map(extract_list_items(first_content));

	std::vector<item_count_map_t> non_first_maps;
	std::string winning_content;

	for (size_t vi = 1; vi < versions.size(); ++vi)
	{
		non_first_maps.push_back(build_item_count_map(extract_list_items(versions[vi])));
		winning_content = versions[vi];
	}

	if (non_first_maps.empty())
		return { false, {} };

	auto merged = build_merged_items(first_map, non_first_maps);
	sort_merged_items(merged);

	const auto header_part = extract_list_header(winning_content);
	const auto record = build_merged_list_record(input.rec_type, header_part, merged);

	if (record == winning_content)
		return { false, winning_content };

	return { true, record };
}

void plugin_scan_t::merge_dialogue(const conflict_entry_t & entry)
{
	if (entry.rec_type != "DIAL")
		return;

	auto get_content = [&](const record_version_t & ver) -> std::string
	{
		if (ver.plugin_idx == m_merge_plugin_idx)
			return m_merge_records[ver.record_index].content;

		m_plugins[ver.plugin_idx]->esm.select_record(ver.record_index);
		return m_plugins[ver.plugin_idx]->esm.get_record().content;
	};

	std::string winning_dial = get_content(entry.versions.back());
	copy_record_to_merge_raw("DIAL", entry.record_id, winning_dial);

	std::vector<std::string> merged_info_ids;
	std::map<std::string, std::string> info_contents;

	for (size_t vi = 0; vi < entry.versions.size(); ++vi)
	{
		const auto & ver = entry.versions[vi];
		if (ver.plugin_idx == m_merge_plugin_idx)
			continue;

		int pi = ver.plugin_idx;
		size_t dial_rec_idx = ver.record_index;

		const auto & plugin_entries = m_plugins[pi]->index.entries();
		for (size_t ei = dial_rec_idx + 1; ei < plugin_entries.size(); ++ei)
		{
			if (plugin_entries[ei].rec_type != "INFO")
				break;

			if (plugin_entries[ei].dial_name != entry.record_id)
				break;

			const auto & info_id = plugin_entries[ei].record_id;

			m_plugins[pi]->esm.select_record(plugin_entries[ei].record_index);
			std::string content = m_plugins[pi]->esm.get_record().content;

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
	const auto & hedr_data = build_hedr_data(m_merge_records.size(), author, description);

	std::string body;
	body += "HEDR";
	body += tools_t::convert_uint_to_string_byte_array(tes3_hedr_size);
	body += hedr_data;

	for (int i = 0; i < static_cast<int>(m_plugins.size()); ++i)
	{
		if (i == m_merge_plugin_idx)
			continue;

		std::string filename = plugin_filename(i);
		filename.push_back('\0');

		body += "MAST";
		body += tools_t::convert_uint_to_string_byte_array(filename.size());
		body += filename;

		uint64_t file_size = 0;
		try
		{
			file_size = std::filesystem::file_size(m_plugins[i]->path);
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
