#include "conflict_slots_cell.hpp"
#include <algorithm>
#include <cstring>
#include <unordered_map>

struct cell_sub_slot_t
{
	std::string type;
	int occurrence;
};

struct cell_ref_group_t
{
	uint32_t object_index;
	size_t start_idx;
	size_t end_idx;
};

static void extract_cell_refs(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    std::vector<std::vector<cell_ref_group_t>> & ver_refs,
    std::vector<size_t> & ver_header_end,
    std::vector<uint32_t> & all_object_indices)
{
	const size_t ver_count = parsed.size();
	ver_refs.resize(ver_count);
	ver_header_end.resize(ver_count, 0);

	for (size_t i = 0; i < ver_count; ++i)
	{
		for (size_t j = 0; j < parsed[i].size(); ++j)
		{
			if (parsed[i][j].type != "FRMR")
				continue;

			if (ver_refs[i].empty())
				ver_header_end[i] = j;

			uint32_t obj_idx = 0;
			if (parsed[i][j].size >= 4)
				std::memcpy(&obj_idx, parsed[i][j].data, 4);

			size_t end_pos = parsed[i].size();
			for (size_t k = j + 1; k < parsed[i].size(); ++k)
			{
				if (parsed[i][k].type == "FRMR")
				{
					end_pos = k;
					break;
				}
			}

			ver_refs[i].push_back({ obj_idx, j, end_pos });

			bool found = false;
			for (const auto & oi : all_object_indices)
			{
				if (oi == obj_idx)
				{
					found = true;
					break;
				}
			}

			if (!found)
				all_object_indices.push_back(obj_idx);
		}

		if (ver_refs[i].empty())
			ver_header_end[i] = parsed[i].size();
	}
}

static void align_cell_header(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    const std::vector<size_t> & ver_header_end,
    slot_result_t & result)
{
	const size_t ver_count = parsed.size();

	std::vector<cell_sub_slot_t> header_slots;
	for (size_t i = 0; i < ver_count; ++i)
	{
		std::unordered_map<std::string, int> type_count;
		for (size_t j = 0; j < ver_header_end[i]; ++j)
		{
			const auto & sv = parsed[i][j];
			int occ = type_count[sv.type]++;
			bool found = false;
			for (const auto & slot : header_slots)
			{
				if (slot.type == sv.type && slot.occurrence == occ)
				{
					found = true;
					break;
				}
			}

			if (!found)
				header_slots.push_back({ sv.type, occ });
		}
	}

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> ver_header_indices(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (size_t j = 0; j < ver_header_end[i]; ++j)
			ver_header_indices[i][parsed[i][j].type].push_back(j);
	}

	for (const auto & slot : header_slots)
	{
		aligned_slot_t aligned;
		aligned.key.type = slot.type;
		aligned.key.occurrence = slot.occurrence;
		aligned.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			auto & indices = ver_header_indices[i][slot.type];
			if (slot.occurrence >= static_cast<int>(indices.size()))
				aligned.indices[i] = SIZE_MAX;
			else
				aligned.indices[i] = indices[slot.occurrence];
		}

		result.aligned.push_back(std::move(aligned));
	}
}

static void collect_ref_group_slots(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    const std::vector<std::vector<cell_ref_group_t>> & ver_refs,
    uint32_t object_index,
    std::vector<cell_sub_slot_t> & ref_slots)
{
	const size_t ver_count = parsed.size();
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (const auto & ref : ver_refs[i])
		{
			if (ref.object_index != object_index)
				continue;

			std::unordered_map<std::string, int> type_count;
			const size_t safe_end = std::min(ref.end_idx, parsed[i].size());
			for (size_t j = ref.start_idx; j < safe_end; ++j)
			{
				const auto & sv = parsed[i][j];
				int occ = type_count[sv.type]++;
				bool found = false;
				for (const auto & slot : ref_slots)
				{
					if (slot.type == sv.type && slot.occurrence == occ)
					{
						found = true;
						break;
					}
				}

				if (!found)
					ref_slots.push_back({ sv.type, occ });
			}

			break;
		}
	}
}

static void align_ref_group_slots(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    const std::vector<std::vector<cell_ref_group_t>> & ver_refs,
    uint32_t object_index,
    const std::vector<cell_sub_slot_t> & ref_slots,
    slot_result_t & result)
{
	const size_t ver_count = parsed.size();

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> ver_ref_indices(ver_count);
	for (size_t i = 0; i < ver_count; ++i)
	{
		for (const auto & ref : ver_refs[i])
		{
			if (ref.object_index != object_index)
				continue;

			const size_t safe_end = std::min(ref.end_idx, parsed[i].size());
			for (size_t j = ref.start_idx; j < safe_end; ++j)
				ver_ref_indices[i][parsed[i][j].type].push_back(j);

			break;
		}
	}

	for (const auto & slot : ref_slots)
	{
		aligned_slot_t aligned;
		aligned.key.type = slot.type;
		aligned.key.occurrence = slot.occurrence;
		aligned.indices.resize(ver_count);

		for (size_t i = 0; i < ver_count; ++i)
		{
			auto & indices = ver_ref_indices[i][slot.type];
			if (slot.occurrence >= static_cast<int>(indices.size()))
				aligned.indices[i] = SIZE_MAX;
			else
				aligned.indices[i] = indices[slot.occurrence];
		}

		result.aligned.push_back(std::move(aligned));
	}
}

static void align_cell_ref_group(
    const std::vector<std::vector<sub_record_view_t>> & parsed,
    const std::vector<std::vector<cell_ref_group_t>> & ver_refs,
    uint32_t object_index,
    slot_result_t & result)
{
	std::vector<cell_sub_slot_t> ref_slots;
	collect_ref_group_slots(parsed, ver_refs, object_index, ref_slots);
	align_ref_group_slots(parsed, ver_refs, object_index, ref_slots, result);
}

void build_cell_slots(slot_result_t & result)
{
	std::vector<std::vector<cell_ref_group_t>> ver_refs;
	std::vector<size_t> ver_header_end;
	std::vector<uint32_t> all_object_indices;
	extract_cell_refs(result.parsed, ver_refs, ver_header_end, all_object_indices);

	align_cell_header(result.parsed, ver_header_end, result);

	for (const auto & obj_idx : all_object_indices)
		align_cell_ref_group(result.parsed, ver_refs, obj_idx, result);
}
