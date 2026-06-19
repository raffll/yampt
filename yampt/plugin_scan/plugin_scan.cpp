#include "plugin_scan.hpp"
#include <algorithm>
#include <filesystem>
#include <set>
#include <cstring>

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

			std::string key = rec.rec_type + std::string(1, '\0') + rec.record_id;
			auto it = entry_lookup_.find(key);

			if (it == entry_lookup_.end())
			{
				conflict_entry_t entry;
				entry.rec_type = rec.rec_type;
				entry.record_id = rec.record_id;
				entry.display_name = rec.display_name;
				entry.dial_name = rec.dial_name;
				entry.conflict_all = conflict_all_t::only_one;

				record_version_t ver;
				ver.plugin_idx = pi;
				ver.record_index = rec.record_index;
				ver.status = conflict_this_t::master;
				entry.versions.push_back(ver);

				entry_lookup_[key] = entries_.size();
				entries_.push_back(std::move(entry));
			}
			else
			{
				auto & entry = entries_[it->second];

				record_version_t ver;
				ver.plugin_idx = pi;
				ver.record_index = rec.record_index;
				ver.status = conflict_this_t::unknown;
				entry.versions.push_back(ver);

				if (entry.display_name.empty() && !rec.display_name.empty())
					entry.display_name = rec.display_name;
			}
		}
	}

	for (auto & entry : entries_)
	{
		if (entry.versions.size() >= 2)
			compute_conflict(entry);
	}
}

void plugin_scan_t::compute_conflict(conflict_entry_t & entry)
{
	auto & master_ver = entry.versions[0];
	plugins_[master_ver.plugin_idx]->esm.select_record(master_ver.record_index);
	const std::string master_content = plugins_[master_ver.plugin_idx]->esm.get_record().content;

	bool any_differs = false;
	int last_differ_idx = -1;
	int differ_count = 0;

	for (size_t i = 1; i < entry.versions.size(); ++i)
	{
		auto & ver = entry.versions[i];

		const auto & plugin_entries = plugins_[ver.plugin_idx]->index.entries();
		if (ver.record_index < plugin_entries.size() && plugin_entries[ver.record_index].has_dele)
		{
			ver.status = conflict_this_t::deleted;
			any_differs = true;
			last_differ_idx = static_cast<int>(i);
			++differ_count;
			continue;
		}

		plugins_[ver.plugin_idx]->esm.select_record(ver.record_index);
		const auto & ver_content = plugins_[ver.plugin_idx]->esm.get_record().content;

		if (content_equal(master_content, ver_content))
		{
			ver.status = conflict_this_t::identical_to_master;
		}
		else
		{
			any_differs = true;
			last_differ_idx = static_cast<int>(i);
			++differ_count;
		}
	}

	if (!any_differs)
	{
		entry.conflict_all = conflict_all_t::no_conflict;
		return;
	}

	if (differ_count == 1)
	{
		entry.conflict_all = conflict_all_t::override_benign;
		auto & ver = entry.versions[last_differ_idx];
		if (ver.status != conflict_this_t::deleted)
			ver.status = conflict_this_t::override_wins;

		return;
	}

	entry.conflict_all = conflict_all_t::conflict;
	for (size_t i = 1; i < entry.versions.size(); ++i)
	{
		auto & ver = entry.versions[i];
		if (ver.status == conflict_this_t::identical_to_master)
			continue;

		if (ver.status == conflict_this_t::deleted)
			continue;

		if (static_cast<int>(i) == last_differ_idx)
			ver.status = conflict_this_t::conflict_wins;
		else
			ver.status = conflict_this_t::conflict_loses;
	}
}

bool plugin_scan_t::content_equal(const std::string & a, const std::string & b) const
{
	if (a.size() != b.size())
		return false;

	if (a.size() < 12)
		return a == b;

	if (a.compare(0, 8, b, 0, 8) != 0)
		return false;

	if (a.compare(12, a.size() - 12, b, 12, b.size() - 12) != 0)
		return false;

	return true;
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
	auto it = std::remove_if(merge_records_.begin(), merge_records_.end(),
	    [&](const merge_record_t & mr)
	    {
		    return mr.rec_type == type && mr.record_id == id;
	    });
	merge_records_.erase(it, merge_records_.end());
}

bool plugin_scan_t::save_merge(const std::string & output_path, const std::string & author, const std::string & description)
{
	if (merge_plugin_idx_ < 0)
		return false;

	std::string header_content = build_tes3_header(author, description);

	std::vector<tools_t::record_t> records;
	records.push_back({"TES3", header_content, header_content.size(), false});

	for (const auto & mr : merge_records_)
		records.push_back({mr.rec_type, mr.content, mr.content.size(), false});

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

std::string plugin_scan_t::build_tes3_header(const std::string & author, const std::string & description)
{
	std::string hedr_data(300, '\0');

	float version = 1.3f;
	std::memcpy(&hedr_data[0], &version, 4);

	uint32_t flags = 0;
	std::memcpy(&hedr_data[4], &flags, 4);

	for (size_t i = 0; i < author.size() && i < 32; ++i)
		hedr_data[8 + i] = author[i];

	for (size_t i = 0; i < description.size() && i < 256; ++i)
		hedr_data[40 + i] = description[i];

	uint32_t num_records = static_cast<uint32_t>(merge_records_.size());
	std::memcpy(&hedr_data[296], &num_records, 4);

	std::string body;
	body += "HEDR";
	body += tools_t::convert_uint_to_string_byte_array(300);
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
		{
		}

		std::string size_data(8, '\0');
		std::memcpy(&size_data[0], &file_size, 8);

		body += "DATA";
		body += tools_t::convert_uint_to_string_byte_array(8);
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
