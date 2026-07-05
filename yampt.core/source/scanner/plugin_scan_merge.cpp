#include "plugin_scan.hpp"
#include <algorithm>

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

void plugin_scan_t::recompute_single_conflict(const std::string & rec_type, const std::string & record_id)
{
	const std::string lookup_key = rec_type + std::string(1, '\0') + record_id;
	auto it_entry = m_entry_lookup.find(lookup_key);
	if (it_entry == m_entry_lookup.end())
	{
		for (size_t mi = 0; mi < m_merge_records.size(); ++mi)
		{
			const auto & merge_rec = m_merge_records[mi];
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

	for (size_t mi = 0; mi < m_merge_records.size(); ++mi)
	{
		const auto & merge_rec = m_merge_records[mi];
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
