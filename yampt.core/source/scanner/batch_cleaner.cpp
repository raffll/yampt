#include "batch_cleaner.hpp"
#include "../decoder/sub_record_iter.hpp"
#include "../utility/string_utils.hpp"
#include "plugin_scan.hpp"
#include <filesystem>
#include <fstream>
#include <set>
#include <unordered_set>

static const std::unordered_set<std::string> & evil_gmst_ids()
{
	static const std::unordered_set<std::string> ids = {
		"sCompanionShare",
		"sCompanionWarningButtonOne",
		"sCompanionWarningButtonTwo",
		"sCompanionWarningMessage",
		"sDeleteNote",
		"sEffectSummonFabricant",
		"sLevitateDisabled",
		"sMagicFabricantID",
		"sMaxSale",
		"sProfitValue",
		"sTeleportDisabled",
		"fCombatDistanceWerewolfMod",
		"fFleeDistance",
		"fWerewolfAcrobatics",
		"fWerewolfAgility",
		"fWerewolfAlchemy",
		"fWerewolfAlteration",
		"fWerewolfArmorer",
		"fWerewolfAthletics",
		"fWerewolfAxe",
		"fWerewolfBlock",
		"fWerewolfBluntWeapon",
		"fWerewolfConjuration",
		"fWerewolfDestruction",
		"fWerewolfEnchant",
		"fWerewolfEndurance",
		"fWerewolfFatigue",
		"fWerewolfHandtoHand",
		"fWerewolfHealth",
		"fWerewolfHeavyArmor",
		"fWerewolfIllusion",
		"fWerewolfIntellegence",
		"fWerewolfLightArmor",
		"fWerewolfLongBlade",
		"fWerewolfLuck",
		"fWerewolfMagicka",
		"fWerewolfMarksman",
		"fWerewolfMediumArmor",
		"fWerewolfMerchantile",
		"fWerewolfMysticism",
		"fWerewolfPersonality",
		"fWerewolfRestoration",
		"fWerewolfRunMult",
		"fWerewolfSecurity",
		"fWerewolfShortBlade",
		"fWerewolfSilverWeaponDamageMult",
		"fWerewolfSneak",
		"fWerewolfSpear",
		"fWerewolfSpeechcraft",
		"fWerewolfSpeed",
		"fWerewolfStrength",
		"fWerewolfUnarmored",
		"fWerewolfWillpower",
		"iWerewolfBounty",
		"iWerewolfFightMod",
		"iWerewolfFleeMod",
		"iWerewolfLevelToAttack",
		"sEditNote",
		"sEffectSummonCreature01",
		"sEffectSummonCreature02",
		"sEffectSummonCreature03",
		"sEffectSummonCreature04",
		"sEffectSummonCreature05",
		"sMagicCreature01ID",
		"sMagicCreature02ID",
		"sMagicCreature03ID",
		"sMagicCreature04ID",
		"sMagicCreature05ID",
		"sWerewolfAlarmMessage",
		"sWerewolfPopup",
		"sWerewolfRefusal",
		"sWerewolfRestMessage",
	};
	return ids;
}

static std::string extract_gmst_id(const std::string & record_content)
{
	sub_record_iter_t iter(record_content);
	sub_record_view_t view;

	while (iter.next(view))
	{
		if (view.type == "NAME")
			return string_utils::erase_null_chars(std::string(view.data, view.size));
	}

	return {};
}

batch_cleaner_t::batch_cleaner_t(plugin_scan_t & scan, log_fn_t log_fn)
    : m_scan(scan)
    , m_log(std::move(log_fn))
{}

bool batch_cleaner_t::is_evil_gmst(const std::string & record_id, const std::string & record_content)
{
	if (record_content.size() < 16)
		return false;

	if (record_content.substr(0, 4) != "GMST")
		return false;

	const auto & ids = evil_gmst_ids();
	auto search_id = record_id;
	if (search_id.empty())
		search_id = extract_gmst_id(record_content);

	return ids.contains(search_id);
}

bool batch_cleaner_t::is_junk_cell(const std::string & record_content)
{
	if (record_content.size() < 16)
		return false;

	if (record_content.substr(0, 4) != "CELL")
		return false;

	static const std::set<std::string> allowed_subs = { "NAME", "DATA", "RGNN" };

	sub_record_iter_t iter(record_content);
	sub_record_view_t view;
	bool has_data = false;
	bool is_interior = false;

	while (iter.next(view))
	{
		if (!allowed_subs.contains(view.type))
			return false;

		if (view.type == "DATA" && view.size >= 4)
		{
			has_data = true;
			const auto flags = static_cast<uint8_t>(view.data[0]);
			is_interior = (flags & 0x01) != 0;
		}
	}

	if (is_interior)
		return false;

	return has_data;
}

bool batch_cleaner_t::is_master_plugin(int plugin_idx) const
{
	const auto filename = m_scan.plugin_filename(plugin_idx);
	const auto lower = string_utils::to_lower(filename);
	const auto extension = std::filesystem::path(lower).extension().string();
	return extension == ".esm";
}

std::vector<clean_result_t> batch_cleaner_t::clean_all(const std::string & output_directory)
{
	std::vector<clean_result_t> results;
	const auto plugin_count = static_cast<int>(m_scan.plugin_count());

	for (int plugin_idx = 0; plugin_idx < plugin_count; ++plugin_idx)
	{
		if (m_scan.is_merge_plugin(plugin_idx))
			continue;

		if (is_master_plugin(plugin_idx))
			continue;

		auto result = clean_plugin(plugin_idx, output_directory);
		if (result.total_removed > 0)
			results.push_back(std::move(result));
	}

	return results;
}

static std::string extract_record_name(const record_t & record)
{
	sub_record_iter_t iter(record.content);
	sub_record_view_t view;

	while (iter.next(view))
	{
		if (view.type == "NAME")
			return string_utils::erase_null_chars(std::string(view.data, view.size));
	}

	return {};
}

static std::set<size_t> collect_itm_indices(plugin_scan_t & scan, int plugin_idx)
{
	std::set<size_t> indices;
	const auto entries = scan.itm_entries(plugin_idx);

	for (const auto * entry : entries)
	{
		for (const auto & version : entry->versions)
		{
			if (version.plugin_idx != plugin_idx)
				continue;

			indices.insert(version.record_index);
			break;
		}
	}

	return indices;
}

clean_result_t batch_cleaner_t::clean_plugin(int plugin_idx, const std::string & output_directory)
{
	clean_result_t result;
	result.plugin_filename = m_scan.plugin_filename(plugin_idx);

	const auto & esm = m_scan.plugin(plugin_idx);
	const auto & records = esm.get_records();
	const auto itm_record_indices = collect_itm_indices(m_scan, plugin_idx);

	std::vector<const record_t *> kept_records;
	kept_records.reserve(records.size());
	std::vector<std::string> removed_log;

	for (size_t record_idx = 0; record_idx < records.size(); ++record_idx)
	{
		const auto & record = records[record_idx];

		if (record.id == "TES3")
		{
			kept_records.push_back(&record);
			continue;
		}

		const auto record_name = extract_record_name(record);

		if (record.id == "GMST" && is_evil_gmst(record_name, record.content))
		{
			removed_log.push_back("  evil GMST: " + record_name);
			++result.evil_gmst_removed;
			continue;
		}

		if (record.id == "CELL" && is_junk_cell(record.content))
		{
			removed_log.push_back("  junk cell: " + record_name);
			++result.junk_cell_removed;
			continue;
		}

		if (itm_record_indices.contains(record_idx))
		{
			removed_log.push_back("  ITM: " + record.id + " " + record_name);
			++result.itm_removed;
			continue;
		}

		kept_records.push_back(&record);
	}

	result.total_removed = result.itm_removed + result.evil_gmst_removed + result.junk_cell_removed;

	if (result.total_removed == 0)
		return result;

	m_log("[info] " + result.plugin_filename + ": removed " + std::to_string(result.total_removed) + " records");
	for (const auto & line : removed_log)
		m_log(line);

	const auto output_path = output_directory + "/" + result.plugin_filename;
	std::filesystem::create_directories(output_directory);

	std::ofstream file(output_path, std::ios::binary);
	if (!file.is_open())
	{
		m_log("[error] cannot write " + output_path);
		return result;
	}

	for (const auto * record : kept_records)
		file << record->content;

	file.close();
	result.written = true;
	return result;
}
