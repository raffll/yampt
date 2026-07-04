#include "plugin_scan.hpp"
#include "sub_record_merge.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <map>

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

	auto plugin = std::make_unique<loaded_plugin_t>();
	plugin->path = filename;
	m_plugins.push_back(std::move(plugin));

	m_merge_records.clear();
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

void plugin_scan_t::clear_merge_records()
{
	m_merge_records.clear();
}

std::vector<plugin_scan_t::merge_record_t> plugin_scan_t::collect_pinned_records() const
{
	std::vector<merge_record_t> pinned;
	for (const auto & record : m_merge_records)
	{
		if (record.pinned)
			pinned.push_back(record);
	}
	return pinned;
}

void plugin_scan_t::restore_pinned_records(const std::vector<merge_record_t> & pinned)
{
	for (const auto & pinned_record : pinned)
	{
		bool replaced = false;
		for (auto & existing : m_merge_records)
		{
			if (existing.rec_type == pinned_record.rec_type && existing.record_id == pinned_record.record_id)
			{
				existing.content = pinned_record.content;
				existing.pinned = true;
				replaced = true;
				break;
			}
		}

		if (!replaced)
			m_merge_records.push_back(pinned_record);
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
	copy_record_to_merge_raw(indexed.rec_type, indexed.record_id, rec.content);
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

	m_merge_records.push_back({ rec_type, record_id, content, false });
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

	m_merge_records.push_back({ rec_type, record_id, content, true });
}

bool plugin_scan_t::is_merge_pinned(const std::string & rec_type, const std::string & record_id) const
{
	for (const auto & record : m_merge_records)
	{
		if (record.rec_type == rec_type && record.record_id == record_id)
			return record.pinned;
	}
	return false;
}

const std::string * plugin_scan_t::find_merge_content(const std::string & rec_type, const std::string & record_id) const
{
	for (const auto & record : m_merge_records)
	{
		if (record.rec_type == rec_type && record.record_id == record_id)
			return &record.content;
	}
	return nullptr;
}

void plugin_scan_t::remove_from_merge(const std::string & type, const std::string & id)
{
	auto it = std::remove_if(
	    m_merge_records.begin(),
	    m_merge_records.end(),
	    [&](const merge_record_t & record) { return record.rec_type == type && record.record_id == id; });
	m_merge_records.erase(it, m_merge_records.end());
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

void plugin_scan_t::merge_leveled_list(const conflict_entry_t & entry)
{
	if (entry.versions.size() < 2)
		return;

	std::vector<std::string> version_contents;
	for (const auto & ver : entry.versions)
	{
		if (ver.plugin_idx == m_merge_plugin_idx)
			continue;

		version_contents.push_back(read_record_content(ver.plugin_idx, ver.record_index));
	}

	merge_input_t input;
	input.rec_type = entry.rec_type;
	input.record_id = entry.record_id;
	input.version_contents = std::move(version_contents);

	const auto result = leveled_list_merge_t::merge(input);
	if (!result.content.empty())
		copy_record_to_merge_raw(entry.rec_type, entry.record_id, result.content);
}

void plugin_scan_t::merge_dialogue(const conflict_entry_t & entry)
{
	if (entry.rec_type != "DIAL")
		return;

	const auto & winning_ver = entry.versions.back();
	std::string winning_dial = read_record_content(winning_ver.plugin_idx, winning_ver.record_index);
	copy_record_to_merge_raw("DIAL", entry.record_id, winning_dial);

	std::vector<std::string> merged_info_ids;
	std::map<std::string, std::string> info_contents;

	for (const auto & ver : entry.versions)
	{
		if (ver.plugin_idx == m_merge_plugin_idx)
			continue;

		const auto & plugin_entries = m_plugins[ver.plugin_idx]->index.entries();
		for (size_t ei = ver.record_index + 1; ei < plugin_entries.size(); ++ei)
		{
			if (plugin_entries[ei].rec_type != "INFO")
				break;

			if (plugin_entries[ei].dial_name != entry.record_id)
				break;

			const auto & info_id = plugin_entries[ei].record_id;
			std::string content = read_record_content(ver.plugin_idx, plugin_entries[ei].record_index);

			if (info_contents.find(info_id) == info_contents.end())
				merged_info_ids.push_back(info_id);

			info_contents[info_id] = content;
		}
	}

	for (const auto & info_id : merged_info_ids)
		copy_record_to_merge_raw("INFO", info_id, info_contents[info_id]);
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

	const auto header_content = build_tes3_header(author, description);

	std::vector<tools_t::record_t> records;
	records.push_back({ "TES3", header_content, header_content.size(), false });

	for (const auto & record : m_merge_records)
		records.push_back({ record.rec_type, record.content, record.content.size(), false });

	const auto temp_path = output_path + ".tmp";
	tools_t::write_file(records, temp_path);

	if (!std::filesystem::exists(temp_path))
		return false;

	std::error_code error_code;
	std::filesystem::rename(temp_path, output_path, error_code);

	if (!error_code)
		return true;

	std::filesystem::copy_file(
	    temp_path,
	    output_path,
	    std::filesystem::copy_options::overwrite_existing,
	    error_code);

	std::filesystem::remove(temp_path, error_code);
	return std::filesystem::exists(output_path);
}

std::string plugin_scan_t::build_tes3_header(const std::string & author, const std::string & description)
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

	uint32_t num_records = static_cast<uint32_t>(m_merge_records.size());
	std::memcpy(&hedr_data[tes3_numrec_offset], &num_records, 4);

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
	header += std::string(4, '\0');
	header += std::string(4, '\0');
	header += body;

	return header;
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
