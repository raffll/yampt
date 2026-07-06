#include "plugin_scan.hpp"
#include <algorithm>

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
