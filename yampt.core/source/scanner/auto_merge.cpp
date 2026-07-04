#include "auto_merge.hpp"
#include "cell_name_fixer.hpp"
#include "fog_fixer.hpp"
#include "plugin_scan.hpp"
#include "summon_fixer.hpp"
#include <regex>
#include <unordered_map>

auto_merge_t::auto_merge_t(plugin_scan_t & scan)
    : m_scan(scan)
{}

void auto_merge_t::set_config(const merge_config_t & config)
{
	m_config = config;
}

merge_counters_t auto_merge_t::execute()
{
	m_log.clear();
	m_scan.clear_merge_records();

	merge_counters_t counters {};
	build_record_groups();
	process_groups(counters);
	apply_fixes(counters);
	prune_unchanged();

	add_log(
	    "[info] merge: " + std::to_string(counters.three_way) + " merged, " + std::to_string(counters.lists) +
	    " lists, " + std::to_string(counters.dialogues) + " dialogues, " + std::to_string(counters.fixes) + " fixes");

	return counters;
}

const std::vector<merge_log_entry_t> & auto_merge_t::log_entries() const
{
	return m_log;
}

void auto_merge_t::build_record_groups()
{
	m_groups.clear();
	std::unordered_map<std::string, size_t> lookup;

	for (int pi = 0; pi < static_cast<int>(m_scan.plugin_count()); ++pi)
	{
		if (!is_plugin_included(pi))
			continue;

		const auto & idx = m_scan.index(pi);
		for (const auto & rec : idx.entries())
		{
			if (rec.rec_type == "TES3")
				continue;

			const auto key = rec.rec_type + "\x00" + rec.record_id;
			auto it_found = lookup.find(key);

			if (it_found == lookup.end())
			{
				lookup[key] = m_groups.size();
				m_groups.push_back({ rec.rec_type, rec.record_id, { { pi, rec.record_index } } });
			}
			else
			{
				m_groups[it_found->second].versions.push_back({ pi, rec.record_index });
			}
		}
	}
}

void auto_merge_t::process_groups(merge_counters_t & counters)
{
	std::regex exclusion_regex;
	bool has_exclusion = false;

	if (!m_config.exclusion_pattern.empty())
	{
		try
		{
			exclusion_regex = std::regex(m_config.exclusion_pattern, std::regex::icase);
			has_exclusion = true;
		}
		catch (...)
		{
			add_log("[error] invalid exclusion regex: " + m_config.exclusion_pattern);
		}
	}

	for (const auto & group : m_groups)
	{
		if (group.versions.size() < 2)
			continue;

		if (!is_type_enabled(group.rec_type))
			continue;

		if (group.rec_type == "INFO")
			continue;

		if (has_exclusion && std::regex_search(group.record_id, exclusion_regex))
			continue;

		const bool is_leveled = (group.rec_type == "LEVI" || group.rec_type == "LEVC");
		const bool is_dialogue = (group.rec_type == "DIAL");

		if (!is_leveled && !is_dialogue && group.versions.size() < 3)
			continue;

		if (is_leveled || is_dialogue)
		{
			const auto & first_ver = group.versions.front();
			const auto & last_ver = group.versions.back();
			const auto first_content = m_scan.read_record_content(first_ver.plugin_idx, first_ver.record_index);
			const auto last_content = m_scan.read_record_content(last_ver.plugin_idx, last_ver.record_index);

			if (first_content == last_content)
				continue;
		}

		try
		{
			if (is_leveled)
				process_leveled_list(group, counters);
			else if (is_dialogue)
				process_dialogue(group, counters);
			else
				process_three_way(group, counters);
		}
		catch (const std::exception & error)
		{
			add_log("[error] merge " + group.rec_type + " \"" + group.record_id + "\": " + error.what());
		}
		catch (...)
		{
			add_log("[error] merge " + group.rec_type + " \"" + group.record_id + "\"");
		}
	}
}

void auto_merge_t::process_leveled_list(const record_group_t & group, merge_counters_t & counters)
{
	auto contents = read_version_contents(group);

	merge_input_t input;
	input.rec_type = group.rec_type;
	input.record_id = group.record_id;
	input.version_contents = std::move(contents);

	const auto result = leveled_list_merge_t::merge(input);
	if (!result.changed)
		return;

	m_scan.copy_record_to_merge_raw(group.rec_type, group.record_id, result.content);
	++counters.lists;
}

void auto_merge_t::process_dialogue(const record_group_t & group, merge_counters_t & counters)
{
	const auto * scan_entry = m_scan.find(group.rec_type, group.record_id);
	if (!scan_entry)
		return;

	m_scan.merge_dialogue(*scan_entry);
	++counters.dialogues;
}

void auto_merge_t::apply_patch_priority(const record_group_t & group, std::vector<std::string> & contents)
{
	if (m_config.patch_plugins.empty())
		return;

	if (contents.size() < 3)
		return;

	const auto & first = contents.front();
	const auto & winner = contents.back();

	if (winner != first)
		return;

	for (size_t v = contents.size() - 2; v >= 1; --v)
	{
		const auto & filename = m_scan.plugin_filename(group.versions[v].plugin_idx);
		if (!m_config.patch_plugins.count(filename))
			continue;

		if (contents[v] == first)
			continue;

		contents.back() = contents[v];
		add_log("[info] patch priority: " + group.rec_type + " \"" + group.record_id + "\" (" + filename + ")");
		return;
	}
}

void auto_merge_t::process_three_way(const record_group_t & group, merge_counters_t & counters)
{
	auto contents = read_version_contents(group);
	apply_patch_priority(group, contents);

	merge_input_t input;
	input.rec_type = group.rec_type;
	input.record_id = group.record_id;
	input.version_contents = std::move(contents);

	for (size_t v = 1; v < group.versions.size() - 1; ++v)
	{
		const auto & filename = m_scan.plugin_filename(group.versions[v].plugin_idx);
		if (m_config.patch_plugins.count(filename))
			input.patch_version_indices.insert(v);
	}

	const auto result = sub_record_merge_t::merge(input);
	if (!result.changed)
		return;

	m_scan.copy_record_to_merge_raw(group.rec_type, group.record_id, result.content);
	++counters.three_way;

	std::string plugins;
	for (const auto & ver : group.versions)
	{
		if (!plugins.empty())
			plugins += ", ";

		plugins += m_scan.plugin_filename(ver.plugin_idx);
	}

	add_log("[info] merge 3-way: " + group.rec_type + " \"" + group.record_id + "\" (" + plugins + ")");
}

void auto_merge_t::apply_fixes(merge_counters_t & counters)
{
	if (m_config.fog_fix_enabled)
		apply_fog_fixes(counters);

	if (m_config.summon_fix_enabled)
		apply_summon_fixes(counters);

	if (m_config.cell_name_fix_enabled)
		apply_cell_name_fixes(counters);
}

void auto_merge_t::apply_fog_fixes(merge_counters_t & counters)
{
	for (const auto & group : m_groups)
	{
		if (group.rec_type != "CELL")
			continue;

		if (m_scan.is_merge_pinned("CELL", group.record_id))
			continue;

		const auto * merge_content = m_scan.find_merge_content("CELL", group.record_id);
		const auto & last_ver = group.versions.back();
		const auto content =
		    merge_content ? *merge_content : m_scan.read_record_content(last_ver.plugin_idx, last_ver.record_index);

		if (content.empty())
			continue;

		const auto fixed = fog_fixer_t::apply(content);
		if (fixed.empty())
			continue;

		m_scan.copy_record_to_merge_raw("CELL", group.record_id, fixed);
		add_log("[info] fog fix: \"" + group.record_id + "\"");
		++counters.fixes;
	}
}

void auto_merge_t::apply_summon_fixes(merge_counters_t & counters)
{
	for (const auto & group : m_groups)
	{
		if (group.rec_type != "CREA")
			continue;

		if (m_scan.is_merge_pinned("CREA", group.record_id))
			continue;

		const auto * merge_content = m_scan.find_merge_content("CREA", group.record_id);
		const auto & last_ver = group.versions.back();
		const auto content =
		    merge_content ? *merge_content : m_scan.read_record_content(last_ver.plugin_idx, last_ver.record_index);

		if (content.empty())
			continue;

		const auto fixed = summon_fixer_t::apply(group.record_id, content);
		if (fixed.empty())
			continue;

		m_scan.copy_record_to_merge_raw("CREA", group.record_id, fixed);
		add_log("[info] summon fix: \"" + group.record_id + "\"");
		++counters.fixes;
	}
}

void auto_merge_t::apply_cell_name_fixes(merge_counters_t & counters)
{
	for (const auto & group : m_groups)
	{
		if (group.rec_type != "CELL")
			continue;

		if (group.versions.size() < 3)
			continue;

		if (m_scan.is_merge_pinned("CELL", group.record_id))
			continue;

		auto version_contents = read_version_contents(group);

		const auto * merge_content = m_scan.find_merge_content("CELL", group.record_id);
		if (merge_content)
			version_contents.back() = *merge_content;

		const auto fixed = cell_name_fixer_t::apply(version_contents);
		if (fixed.empty())
			continue;

		m_scan.copy_record_to_merge_raw("CELL", group.record_id, fixed);
		add_log("[info] cell name fix: \"" + group.record_id + "\"");
		++counters.fixes;
	}
}

void auto_merge_t::prune_unchanged()
{
	std::unordered_map<std::string, const record_group_t *> group_lookup;
	for (const auto & group : m_groups)
		group_lookup[group.rec_type + "\x00" + group.record_id] = &group;

	std::vector<std::pair<std::string, std::string>> to_remove;

	for (size_t i = 0; i < m_scan.merge_record_count(); ++i)
	{
		const auto & rec_type = m_scan.merge_record_type(i);
		const auto & record_id = m_scan.merge_record_id(i);
		const auto & merge_content = m_scan.merge_record_content(i);

		const auto key = rec_type + "\x00" + record_id;
		auto it_found = group_lookup.find(key);
		if (it_found == group_lookup.end())
			continue;

		const auto & last_ver = it_found->second->versions.back();
		const auto winning = m_scan.read_record_content(last_ver.plugin_idx, last_ver.record_index);

		if (merge_content == winning)
			to_remove.emplace_back(rec_type, record_id);
	}

	for (const auto & [rec_type, record_id] : to_remove)
		m_scan.remove_from_merge(rec_type, record_id);
}

std::vector<std::string> auto_merge_t::read_version_contents(const record_group_t & group)
{
	std::vector<std::string> result;
	result.reserve(group.versions.size());

	for (const auto & ver : group.versions)
		result.push_back(m_scan.read_record_content(ver.plugin_idx, ver.record_index));

	return result;
}

bool auto_merge_t::is_plugin_included(int plugin_idx) const
{
	if (m_scan.is_merge_plugin(plugin_idx))
		return false;

	const auto & filename = m_scan.plugin_filename(plugin_idx);
	return m_config.excluded_plugins.count(filename) == 0;
}

bool auto_merge_t::is_type_enabled(const std::string & rec_type) const
{
	return m_config.disabled_types.count(rec_type) == 0;
}

bool auto_merge_t::matches_exclusion(const std::string & record_id) const
{
	if (m_config.exclusion_pattern.empty())
		return false;

	try
	{
		std::regex pattern(m_config.exclusion_pattern, std::regex::icase);
		return std::regex_search(record_id, pattern);
	}
	catch (...)
	{
		return false;
	}
}

void auto_merge_t::add_log(const std::string & message)
{
	m_log.push_back({ message });
}
