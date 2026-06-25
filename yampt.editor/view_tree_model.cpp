#include "view_tree_model.hpp"
#include "view_tree_format.hpp"
#include "../yampt/plugin_scan/conflict_compute.hpp"
#include <QBrush>
#include <QMimeData>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <map>

view_tree_model_t::view_tree_model_t(QObject * parent)
    : QAbstractItemModel(parent)
{}

void view_tree_model_t::set_record(plugin_scan_t & scan, const conflict_entry_t & entry)
{
	beginResetModel();

	rows_.clear();
	column_names_.clear();
	plugin_conflict_this_.clear();
	record_type_ = entry.rec_type;
	record_id_ = entry.record_id;
	column_plugin_indices_.clear();
	filter_dirty_ = true;
	has_merge_column_ = false;
	merge_col_index_ = -1;

	size_t col_count = entry.versions.size();

	for (const auto & ver : entry.versions)
	{
		char buf[64];
		std::snprintf(buf, sizeof(buf), "[%02X] %s", ver.plugin_idx, scan.plugin_filename(ver.plugin_idx).c_str());
		column_names_.push_back(buf);
		plugin_conflict_this_.push_back(ver.status);
		column_plugin_indices_.push_back(ver.plugin_idx);
	}

	if (scan.has_merge())
	{
		bool merge_already_in_versions = false;
		for (const auto & ver : entry.versions)
		{
			if (scan.is_merge_plugin(ver.plugin_idx))
			{
				merge_already_in_versions = true;
				break;
			}
		}

		if (!merge_already_in_versions)
		{
			has_merge_column_ = true;
			merge_col_index_ = static_cast<int>(col_count);
			int merge_idx = -1;
			for (int i = 0; i < static_cast<int>(scan.plugin_count()); ++i)
			{
				if (scan.is_merge_plugin(i))
				{
					merge_idx = i;
					break;
				}
			}

			char buf[64];
			std::snprintf(buf, sizeof(buf), "[%02X] %s *", merge_idx, scan.plugin_filename(merge_idx).c_str());
			column_names_.push_back(buf);
			plugin_conflict_this_.push_back(conflict_this_t::unknown);
			column_plugin_indices_.push_back(merge_idx);
			++col_count;
		}
	}

	if (col_count == 0)
	{
		endResetModel();
		return;
	}

	{
		sub_record_row_t header_row;
		header_row.type = "Record Header";
		header_row.label = "Record Header";
		header_row.size = 16;
		header_row.values.resize(col_count);
		header_row.row_conflict_all = conflict_all_t::only_one;
		header_row.all_identical = true;
		header_row.cell_conflict_this.resize(col_count, conflict_this_t::master);

		field_row_t sig_row;
		sig_row.name = "Signature";
		sig_row.values.resize(col_count, entry.rec_type);
		sig_row.row_conflict_all = compute_conflict_all(sig_row.values);
		sig_row.all_identical = true;
		sig_row.cell_conflict_this = compute_conflict_this(sig_row.values);
		header_row.children.push_back(std::move(sig_row));

		field_row_t flags_row;
		flags_row.name = "Record Flags";
		flags_row.values.resize(col_count);

		for (size_t col = 0; col < entry.versions.size(); ++col)
		{
			std::string content;
			if (scan.is_merge_plugin(entry.versions[col].plugin_idx))
			{
				if (entry.versions[col].record_index < scan.merge_record_count())
					content = scan.merge_record_content(entry.versions[col].record_index);
			}
			else
			{
				const auto & records = scan.plugin(entry.versions[col].plugin_idx).get_records();
				if (entry.versions[col].record_index < records.size())
					content = records[entry.versions[col].record_index].content;
			}

			if (content.size() >= 16)
			{
				uint32_t flags = 0;
				std::memcpy(&flags, content.data() + 12, 4);
				char fbuf[16];
				std::snprintf(fbuf, sizeof(fbuf), "0x%08X", flags);
				flags_row.values[col] = fbuf;
			}
		}

		flags_row.all_identical = true;
		for (size_t col = 1; col < col_count; ++col)
		{
			if (flags_row.values[col] != flags_row.values[0])
			{
				flags_row.all_identical = false;
				break;
			}
		}
		flags_row.row_conflict_all = compute_conflict_all(flags_row.values);
		flags_row.cell_conflict_this = compute_conflict_this(flags_row.values);
		header_row.children.push_back(std::move(flags_row));

		rows_.push_back(std::move(header_row));
	}

	std::vector<std::vector<sub_record_view_t>> all_sub_records;
	std::vector<std::string> content_storage;

	for (const auto & ver : entry.versions)
	{
		std::string content;

		if (scan.is_merge_plugin(ver.plugin_idx))
		{
			if (ver.record_index < scan.merge_record_count())
				content = scan.merge_record_content(ver.record_index);
		}
		else
		{
			const auto & records = scan.plugin(ver.plugin_idx).get_records();
			if (ver.record_index < records.size())
				content = records[ver.record_index].content;
		}

		if (content.size() < 16)
		{
			all_sub_records.push_back({});
			content_storage.push_back({});
			continue;
		}

		content_storage.push_back(std::move(content));
		const auto & stored = content_storage.back();

		std::vector<sub_record_view_t> subs;
		sub_record_iter_t iter(stored);
		sub_record_view_t sv;
		while (iter.next(sv))
			subs.push_back(sv);

		all_sub_records.push_back(std::move(subs));
	}

	struct sub_slot_t
	{
		std::string type;
		int occurrence;
	};

	bool is_leveled_list = (record_type_ == "LEVI" || record_type_ == "LEVC");
	bool is_cell = (record_type_ == "CELL");

	std::vector<sub_slot_t> unified_slots;

	if (is_cell)
	{
		struct ref_group_t
		{
			uint32_t object_index;
			size_t start_idx;
			size_t end_idx;
		};

		std::vector<std::vector<ref_group_t>> col_refs(col_count);
		std::vector<uint32_t> all_object_indices;
		std::vector<size_t> col_header_end(col_count, 0);

		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
				continue;

			const auto & subs = all_sub_records[col];

			for (size_t i = 0; i < subs.size(); ++i)
			{
				if (subs[i].type != "FRMR")
					continue;

				if (col_refs[col].empty())
					col_header_end[col] = i;

				uint32_t obj_idx = 0;
				if (subs[i].size >= 4)
					std::memcpy(&obj_idx, subs[i].data, 4);

				size_t end = subs.size();
				for (size_t j = i + 1; j < subs.size(); ++j)
				{
					if (subs[j].type == "FRMR")
					{
						end = j;
						break;
					}
				}

				col_refs[col].push_back({ obj_idx, i, end });

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

			if (col_refs[col].empty())
				col_header_end[col] = all_sub_records[col].size();
		}

		std::vector<sub_slot_t> header_slots;
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
				continue;

			std::unordered_map<std::string, int> type_count;
			for (size_t i = 0; i < col_header_end[col]; ++i)
			{
				const auto & sv = all_sub_records[col][i];
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

		std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_header_indices(col_count);
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
				continue;

			for (size_t i = 0; i < col_header_end[col]; ++i)
				col_header_indices[col][all_sub_records[col][i].type].push_back(i);
		}

		for (const auto & slot : header_slots)
		{
			sub_record_row_t row;
			row.size = 0;
			row.values.resize(col_count);
			const char * first_data = nullptr;
			size_t first_size = 0;

			for (size_t col = 0; col < col_count; ++col)
			{
				if (col >= all_sub_records.size())
				{
					row.values[col] = "";
					continue;
				}

				auto & indices = col_header_indices[col][slot.type];
				if (slot.occurrence >= static_cast<int>(indices.size()))
				{
					row.values[col] = "";
					continue;
				}

				const auto & sv = all_sub_records[col][indices[slot.occurrence]];
				if (!first_data)
				{
					first_data = sv.data;
					first_size = sv.size;
					row.size = sv.size;
				}

				row.values[col] = format_value(sv.data, sv.size);
			}

			row.type = slot.type;
			row.label = make_sub_label(slot.type, record_type_, first_size);

			bool all_same = true;
			for (size_t col = 1; col < col_count; ++col)
			{
				if (row.values[col] != row.values[0])
				{
					all_same = false;
					break;
				}
			}
			row.all_identical = all_same;
			row.row_conflict_all = compute_conflict_all(row.values);
			row.cell_conflict_this = compute_conflict_this(row.values);

			const sub_record_schema_t * schema = find_schema(record_type_, slot.type, first_size);
			if (schema && first_data)
			{
				for (size_t fi = 0; fi < schema->field_count; ++fi)
				{
					const auto & fdef = schema->fields[fi];

					const bool is_flags = (fdef.type == field_type_t::flags_u8 ||
					                       fdef.type == field_type_t::flags_u16 ||
					                       fdef.type == field_type_t::flags_u32);

					if (is_flags && fdef.flag_names && fdef.flag_count > 0)
					{
						for (int bit = 0; bit < fdef.flag_count; ++bit)
						{
							if (fdef.flag_names[bit][0] == '_')
								continue;

							field_row_t frow;
							frow.name = fdef.flag_names[bit];
							frow.values.resize(col_count);

							for (size_t col = 0; col < col_count; ++col)
							{
								if (col >= all_sub_records.size())
								{
									frow.values[col] = "";
									continue;
								}

								auto & indices = col_header_indices[col][slot.type];
								if (slot.occurrence >= static_cast<int>(indices.size()))
								{
									frow.values[col] = "";
									continue;
								}

								const auto & sv = all_sub_records[col][indices[slot.occurrence]];
								if (fdef.offset >= sv.size)
								{
									frow.values[col] = "";
									continue;
								}

								uint32_t val = 0;
								size_t bytes = (fdef.type == field_type_t::flags_u8) ? 1 :
								               (fdef.type == field_type_t::flags_u16) ? 2 : 4;
								std::memcpy(&val, sv.data + fdef.offset, std::min(bytes, sv.size - fdef.offset));
								frow.values[col] = (val & (1u << bit)) ? "1" : "0";
							}

							bool fields_same = true;
							for (size_t col = 1; col < col_count; ++col)
							{
								if (frow.values[col] != frow.values[0])
								{
									fields_same = false;
									break;
								}
							}
							frow.all_identical = fields_same;
							frow.row_conflict_all = compute_conflict_all(frow.values);
							frow.cell_conflict_this = compute_conflict_this(frow.values);

							row.children.push_back(std::move(frow));
						}

						continue;
					}

					field_row_t frow;
					frow.name = fdef.name;
					frow.values.resize(col_count);

					for (size_t col = 0; col < col_count; ++col)
					{
						if (col >= all_sub_records.size())
						{
							frow.values[col] = "";
							continue;
						}

						auto & indices = col_header_indices[col][slot.type];
						if (slot.occurrence >= static_cast<int>(indices.size()))
						{
							frow.values[col] = "";
							continue;
						}

						const auto & sv = all_sub_records[col][indices[slot.occurrence]];
						frow.values[col] = decode_field(fdef, sv.data, sv.size);
					}

					bool fields_same = true;
					for (size_t col = 1; col < col_count; ++col)
					{
						if (frow.values[col] != frow.values[0])
						{
							fields_same = false;
							break;
						}
					}
					frow.all_identical = fields_same;
					frow.row_conflict_all = compute_conflict_all(frow.values);
					frow.cell_conflict_this = compute_conflict_this(frow.values);
					row.children.push_back(std::move(frow));
				}
			}
			else if (
			    first_data && first_size > 0 && !row.values.empty() && row.values[0].size() > 0 && row.values[0][0] == '<')
			{
				for (size_t offset = 0; offset < first_size; offset += 16)
				{
					field_row_t frow;
					char name_buf[16];
					std::snprintf(name_buf, sizeof(name_buf), "%04X", static_cast<unsigned>(offset));
					frow.name = name_buf;
					frow.values.resize(col_count);

					for (size_t col = 0; col < col_count; ++col)
					{
						if (col >= all_sub_records.size())
						{
							frow.values[col] = "";
							continue;
						}

						auto & indices = col_header_indices[col][slot.type];
						if (slot.occurrence >= static_cast<int>(indices.size()))
						{
							frow.values[col] = "";
							continue;
						}

						const auto & sv = all_sub_records[col][indices[slot.occurrence]];
						if (offset >= sv.size)
						{
							frow.values[col] = "";
							continue;
						}

						size_t chunk = std::min(static_cast<size_t>(16), sv.size - offset);
						std::string hex;
						for (size_t b = 0; b < chunk; ++b)
						{
							char hbuf[4];
							std::snprintf(hbuf, sizeof(hbuf), "%02X", static_cast<unsigned char>(sv.data[offset + b]));
							if (!hex.empty())
								hex += ' ';

							hex += hbuf;
						}
						frow.values[col] = hex;
					}

					bool fields_same = true;
					for (size_t col = 1; col < col_count; ++col)
					{
						if (frow.values[col] != frow.values[0])
						{
							fields_same = false;
							break;
						}
					}
					frow.all_identical = fields_same;
					frow.row_conflict_all = compute_conflict_all(frow.values);
					frow.cell_conflict_this = compute_conflict_this(frow.values);

					row.children.push_back(std::move(frow));
				}
			}

			rows_.push_back(std::move(row));
		}

		for (const auto & obj_idx : all_object_indices)
		{
			std::vector<sub_slot_t> ref_slots;
			for (size_t col = 0; col < col_count; ++col)
			{
				if (col >= all_sub_records.size())
					continue;

				for (const auto & ref : col_refs[col])
				{
					if (ref.object_index != obj_idx)
						continue;

					std::unordered_map<std::string, int> type_count;
					for (size_t i = ref.start_idx; i < ref.end_idx; ++i)
					{
						const auto & sv = all_sub_records[col][i];
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

			for (const auto & slot : ref_slots)
			{
				sub_record_row_t row;
				row.size = 0;
				row.values.resize(col_count);
				const char * first_data = nullptr;
				size_t first_size = 0;

				for (size_t col = 0; col < col_count; ++col)
				{
					if (col >= all_sub_records.size())
					{
						row.values[col] = "";
						continue;
					}

					bool col_found = false;
					for (const auto & ref : col_refs[col])
					{
						if (ref.object_index != obj_idx)
							continue;

						int occ = 0;
						for (size_t i = ref.start_idx; i < ref.end_idx; ++i)
						{
							const auto & sv = all_sub_records[col][i];
							if (sv.type != slot.type)
								continue;

							if (occ == slot.occurrence)
							{
								if (!first_data)
								{
									first_data = sv.data;
									first_size = sv.size;
									row.size = sv.size;
								}

								row.values[col] = format_value(sv.data, sv.size);
								col_found = true;
								break;
							}

							++occ;
						}

						break;
					}

					if (!col_found)
						row.values[col] = "";
				}

				row.type = slot.type;
				row.label = make_sub_label(slot.type, record_type_, first_size);

				bool all_same = true;
				for (size_t col = 1; col < col_count; ++col)
				{
					if (row.values[col] != row.values[0])
					{
						all_same = false;
						break;
					}
				}
				row.all_identical = all_same;
				row.row_conflict_all = compute_conflict_all(row.values);
				row.cell_conflict_this = compute_conflict_this(row.values);

				const sub_record_schema_t * schema = find_schema(record_type_, slot.type, first_size);
				if (schema && first_data)
				{
					for (size_t fi = 0; fi < schema->field_count; ++fi)
					{
						const auto & fdef = schema->fields[fi];

						const bool is_flags = (fdef.type == field_type_t::flags_u8 ||
						                       fdef.type == field_type_t::flags_u16 ||
						                       fdef.type == field_type_t::flags_u32);

						if (is_flags && fdef.flag_names && fdef.flag_count > 0)
						{
							for (int bit = 0; bit < fdef.flag_count; ++bit)
							{
								if (fdef.flag_names[bit][0] == '_')
									continue;

								field_row_t frow;
								frow.name = fdef.flag_names[bit];
								frow.values.resize(col_count);

								for (size_t col = 0; col < col_count; ++col)
								{
									if (col >= all_sub_records.size())
									{
										frow.values[col] = "";
										continue;
									}

									bool col_found2 = false;
									for (const auto & ref : col_refs[col])
									{
										if (ref.object_index != obj_idx)
											continue;

										int occ = 0;
										for (size_t i = ref.start_idx; i < ref.end_idx; ++i)
										{
											const auto & sv = all_sub_records[col][i];
											if (sv.type != slot.type)
												continue;

											if (occ == slot.occurrence)
											{
												if (fdef.offset >= sv.size)
												{
													frow.values[col] = "";
												}
												else
												{
													uint32_t val = 0;
													size_t bytes = (fdef.type == field_type_t::flags_u8) ? 1 :
													               (fdef.type == field_type_t::flags_u16) ? 2 : 4;
													std::memcpy(&val, sv.data + fdef.offset, std::min(bytes, sv.size - fdef.offset));
													frow.values[col] = (val & (1u << bit)) ? "1" : "0";
												}
												col_found2 = true;
												break;
											}

											++occ;
										}

										break;
									}

									if (!col_found2)
										frow.values[col] = "";
								}

								bool fields_same = true;
								for (size_t col = 1; col < col_count; ++col)
								{
									if (frow.values[col] != frow.values[0])
									{
										fields_same = false;
										break;
									}
								}
								frow.all_identical = fields_same;
								frow.row_conflict_all = compute_conflict_all(frow.values);
								frow.cell_conflict_this = compute_conflict_this(frow.values);

								row.children.push_back(std::move(frow));
							}

							continue;
						}

						field_row_t frow;
						frow.name = fdef.name;
						frow.values.resize(col_count);

						for (size_t col = 0; col < col_count; ++col)
						{
							if (col >= all_sub_records.size())
							{
								frow.values[col] = "";
								continue;
							}

							bool col_found2 = false;
							for (const auto & ref : col_refs[col])
							{
								if (ref.object_index != obj_idx)
									continue;

								int occ = 0;
								for (size_t i = ref.start_idx; i < ref.end_idx; ++i)
								{
									const auto & sv = all_sub_records[col][i];
									if (sv.type != slot.type)
										continue;

									if (occ == slot.occurrence)
									{
										frow.values[col] = decode_field(fdef, sv.data, sv.size);
										col_found2 = true;
										break;
									}

									++occ;
								}

								break;
							}

							if (!col_found2)
								frow.values[col] = "";
						}

						bool fields_same = true;
						for (size_t col = 1; col < col_count; ++col)
						{
							if (frow.values[col] != frow.values[0])
							{
								fields_same = false;
								break;
							}
						}
						frow.all_identical = fields_same;
						frow.row_conflict_all = compute_conflict_all(frow.values);
						frow.cell_conflict_this = compute_conflict_this(frow.values);
						row.children.push_back(std::move(frow));
					}
				}
				else if (
				    first_data && first_size > 0 && !row.values.empty() && row.values[0].size() > 0 && row.values[0][0] == '<')
				{
					for (size_t offset = 0; offset < first_size; offset += 16)
					{
						field_row_t frow;
						char name_buf[16];
						std::snprintf(name_buf, sizeof(name_buf), "%04X", static_cast<unsigned>(offset));
						frow.name = name_buf;
						frow.values.resize(col_count);

						for (size_t col = 0; col < col_count; ++col)
						{
							if (col >= all_sub_records.size())
							{
								frow.values[col] = "";
								continue;
							}

							bool col_found2 = false;
							for (const auto & ref : col_refs[col])
							{
								if (ref.object_index != obj_idx)
									continue;

								int occ = 0;
								for (size_t i = ref.start_idx; i < ref.end_idx; ++i)
								{
									const auto & sv = all_sub_records[col][i];
									if (sv.type != slot.type)
										continue;

									if (occ == slot.occurrence)
									{
										if (offset >= sv.size)
										{
											frow.values[col] = "";
										}
										else
										{
											size_t chunk = std::min(static_cast<size_t>(16), sv.size - offset);
											std::string hex;
											for (size_t b = 0; b < chunk; ++b)
											{
												char hbuf[4];
												std::snprintf(hbuf, sizeof(hbuf), "%02X", static_cast<unsigned char>(sv.data[offset + b]));
												if (!hex.empty())
													hex += ' ';

												hex += hbuf;
											}
											frow.values[col] = hex;
										}
										col_found2 = true;
										break;
									}

									++occ;
								}

								break;
							}

							if (!col_found2)
								frow.values[col] = "";
						}

						bool fields_same = true;
						for (size_t col = 1; col < col_count; ++col)
						{
							if (frow.values[col] != frow.values[0])
							{
								fields_same = false;
								break;
							}
						}
						frow.all_identical = fields_same;
						frow.row_conflict_all = compute_conflict_all(frow.values);
						frow.cell_conflict_this = compute_conflict_this(frow.values);

						row.children.push_back(std::move(frow));
					}
				}

				rows_.push_back(std::move(row));
			}
		}

		if (!rows_.empty())
		{
			conflict_all_t worst = conflict_all_t::only_one;
			for (size_t i = 1; i < rows_.size(); ++i)
			{
				if (rows_[i].row_conflict_all > worst)
					worst = rows_[i].row_conflict_all;
			}
			rows_[0].row_conflict_all = worst;
			rows_[0].all_identical = (worst <= conflict_all_t::no_conflict);
		}

		endResetModel();
		return;
	}
	else if (is_leveled_list)
	{
		struct lev_entry_t
		{
			std::string item_id;
			size_t intv_idx;
			size_t inam_idx;
		};

		std::vector<std::vector<lev_entry_t>> col_entries(col_count);
		std::vector<std::string> all_item_ids;

		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
				continue;

			const auto & subs = all_sub_records[col];
			for (size_t i = 0; i + 1 < subs.size(); ++i)
			{
				if (subs[i].type == "INTV" && subs[i].size == 2 && subs[i + 1].type == "INAM")
				{
					std::string item_id(subs[i + 1].data, subs[i + 1].size);
					if (!item_id.empty() && item_id.back() == '\0')
						item_id.pop_back();

					col_entries[col].push_back({ item_id, i, i + 1 });

					bool found = false;
					for (const auto & id : all_item_ids)
					{
						if (id == item_id)
						{
							found = true;
							break;
						}
					}
					if (!found)
						all_item_ids.push_back(item_id);
				}
			}
		}

		std::unordered_map<std::string, int> type_count;
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
				continue;

			for (const auto & sv : all_sub_records[col])
			{
				if (sv.type == "INTV" && sv.size == 2)
					continue;

				if (sv.type == "INAM")
					continue;

				int occ = type_count[sv.type]++;
				bool found = false;
				for (const auto & slot : unified_slots)
				{
					if (slot.type == sv.type && slot.occurrence == occ)
					{
						found = true;
						break;
					}
				}
				if (!found)
					unified_slots.push_back({ sv.type, occ });
			}
			type_count.clear();
		}

		for (const auto & item_id : all_item_ids)
		{
			int intv_occ = -1;
			int inam_occ = -1;

			for (const auto & slot : unified_slots)
			{
				if (slot.type == "INTV")
					intv_occ = std::max(intv_occ, slot.occurrence);

				if (slot.type == "INAM")
					inam_occ = std::max(inam_occ, slot.occurrence);
			}

			unified_slots.push_back({ "INTV", intv_occ + 1 });
			unified_slots.push_back({ "INAM", inam_occ + 1 });
		}

		std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices_lev(col_count);
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
				continue;

			for (size_t i = 0; i < all_sub_records[col].size(); ++i)
			{
				const auto & sv = all_sub_records[col][i];
				if ((sv.type == "INTV" && sv.size == 2) || sv.type == "INAM")
					continue;

				col_type_indices_lev[col][sv.type].push_back(i);
			}

			for (size_t entry_idx = 0; entry_idx < all_item_ids.size(); ++entry_idx)
			{
				const auto & target_id = all_item_ids[entry_idx];
				bool matched = false;
				for (const auto & e : col_entries[col])
				{
					if (e.item_id == target_id)
					{
						col_type_indices_lev[col]["INTV"].push_back(e.intv_idx);
						col_type_indices_lev[col]["INAM"].push_back(e.inam_idx);
						matched = true;
						break;
					}
				}
				if (!matched)
				{
					col_type_indices_lev[col]["INTV"].push_back(SIZE_MAX);
					col_type_indices_lev[col]["INAM"].push_back(SIZE_MAX);
				}
			}
		}

		std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
		col_type_indices = std::move(col_type_indices_lev);

		for (const auto & slot : unified_slots)
		{
			sub_record_row_t row;
			row.size = 0;
			row.values.resize(col_count);

			std::string sub_type = slot.type;
			const char * first_data = nullptr;
			size_t first_size = 0;

			for (size_t col = 0; col < col_count; ++col)
			{
				if (col >= all_sub_records.size())
				{
					row.values[col] = "";
					continue;
				}

				auto & indices = col_type_indices[col][slot.type];
				if (slot.occurrence >= static_cast<int>(indices.size()))
				{
					row.values[col] = "";
					continue;
				}

				size_t idx = indices[slot.occurrence];
				if (idx == SIZE_MAX)
				{
					row.values[col] = "";
					continue;
				}

				const auto & sv = all_sub_records[col][idx];

				if (!first_data)
				{
					first_data = sv.data;
					first_size = sv.size;
					row.size = sv.size;
				}

				row.values[col] = format_value(sv.data, sv.size);
			}

			row.type = sub_type;
			row.label = make_sub_label(sub_type, record_type_, first_size);

			bool all_same = true;
			for (size_t col = 1; col < col_count; ++col)
			{
				if (row.values[col] != row.values[0])
				{
					all_same = false;
					break;
				}
			}
			row.all_identical = all_same;
			row.row_conflict_all = compute_conflict_all(row.values);
			row.cell_conflict_this = compute_conflict_this(row.values);

			const sub_record_schema_t * schema = find_schema(record_type_, sub_type, first_size);
			if (schema && first_data)
			{
				for (size_t fi = 0; fi < schema->field_count; ++fi)
				{
					const auto & fdef = schema->fields[fi];

					const bool is_flags = (fdef.type == field_type_t::flags_u8 ||
					                       fdef.type == field_type_t::flags_u16 ||
					                       fdef.type == field_type_t::flags_u32);

					if (is_flags && fdef.flag_names && fdef.flag_count > 0)
					{
						for (int bit = 0; bit < fdef.flag_count; ++bit)
						{
							if (fdef.flag_names[bit][0] == '_')
								continue;

							field_row_t frow;
							frow.name = fdef.flag_names[bit];
							frow.values.resize(col_count);

							for (size_t col = 0; col < col_count; ++col)
							{
								if (col >= all_sub_records.size())
								{
									frow.values[col] = "";
									continue;
								}

								auto & indices2 = col_type_indices[col][slot.type];
								if (slot.occurrence >= static_cast<int>(indices2.size()))
								{
									frow.values[col] = "";
									continue;
								}

								size_t idx2 = indices2[slot.occurrence];
								if (idx2 == SIZE_MAX)
								{
									frow.values[col] = "";
									continue;
								}

								const auto & sv = all_sub_records[col][idx2];
								if (fdef.offset >= sv.size)
								{
									frow.values[col] = "";
									continue;
								}

								uint32_t val = 0;
								size_t bytes = (fdef.type == field_type_t::flags_u8) ? 1 :
								               (fdef.type == field_type_t::flags_u16) ? 2 : 4;
								std::memcpy(&val, sv.data + fdef.offset, std::min(bytes, sv.size - fdef.offset));
								frow.values[col] = (val & (1u << bit)) ? "1" : "0";
							}

							bool fields_same = true;
							for (size_t col = 1; col < col_count; ++col)
							{
								if (frow.values[col] != frow.values[0])
								{
									fields_same = false;
									break;
								}
							}
							frow.all_identical = fields_same;
							frow.row_conflict_all = compute_conflict_all(frow.values);
							frow.cell_conflict_this = compute_conflict_this(frow.values);

							row.children.push_back(std::move(frow));
						}

						continue;
					}

					field_row_t frow;
					frow.name = fdef.name;
					frow.values.resize(col_count);

					for (size_t col = 0; col < col_count; ++col)
					{
						if (col >= all_sub_records.size())
						{
							frow.values[col] = "";
							continue;
						}

						auto & indices2 = col_type_indices[col][slot.type];
						if (slot.occurrence >= static_cast<int>(indices2.size()))
						{
							frow.values[col] = "";
							continue;
						}

						size_t idx2 = indices2[slot.occurrence];
						if (idx2 == SIZE_MAX)
						{
							frow.values[col] = "";
							continue;
						}

						const auto & sv = all_sub_records[col][idx2];
						frow.values[col] = decode_field(fdef, sv.data, sv.size);
					}

					bool fields_same = true;
					for (size_t col = 1; col < col_count; ++col)
					{
						if (frow.values[col] != frow.values[0])
						{
							fields_same = false;
							break;
						}
					}
					frow.all_identical = fields_same;
					frow.row_conflict_all = compute_conflict_all(frow.values);
					frow.cell_conflict_this = compute_conflict_this(frow.values);

					row.children.push_back(std::move(frow));
				}
			}
			else if (
			    first_data && first_size > 0 && !row.values.empty() && row.values[0].size() > 0 && row.values[0][0] == '<')
			{
				for (size_t offset = 0; offset < first_size; offset += 16)
				{
					field_row_t frow;
					char name_buf[16];
					std::snprintf(name_buf, sizeof(name_buf), "%04X", static_cast<unsigned>(offset));
					frow.name = name_buf;
					frow.values.resize(col_count);

					for (size_t col = 0; col < col_count; ++col)
					{
						if (col >= all_sub_records.size())
						{
							frow.values[col] = "";
							continue;
						}

						auto & indices2 = col_type_indices[col][slot.type];
						if (slot.occurrence >= static_cast<int>(indices2.size()))
						{
							frow.values[col] = "";
							continue;
						}

						size_t idx2 = indices2[slot.occurrence];
						if (idx2 == SIZE_MAX)
						{
							frow.values[col] = "";
							continue;
						}

						const auto & sv = all_sub_records[col][idx2];
						if (offset >= sv.size)
						{
							frow.values[col] = "";
							continue;
						}

						size_t chunk = std::min(static_cast<size_t>(16), sv.size - offset);
						std::string hex;
						for (size_t b = 0; b < chunk; ++b)
						{
							char hbuf[4];
							std::snprintf(hbuf, sizeof(hbuf), "%02X", static_cast<unsigned char>(sv.data[offset + b]));
							if (!hex.empty())
								hex += ' ';

							hex += hbuf;
						}
						frow.values[col] = hex;
					}

					bool fields_same = true;
					for (size_t col = 1; col < col_count; ++col)
					{
						if (frow.values[col] != frow.values[0])
						{
							fields_same = false;
							break;
						}
					}
					frow.all_identical = fields_same;
					frow.row_conflict_all = compute_conflict_all(frow.values);
					frow.cell_conflict_this = compute_conflict_this(frow.values);

					row.children.push_back(std::move(frow));
				}
			}

			rows_.push_back(std::move(row));
		}

		if (!rows_.empty())
		{
			conflict_all_t worst = conflict_all_t::only_one;
			for (size_t i = 1; i < rows_.size(); ++i)
			{
				if (rows_[i].row_conflict_all > worst)
					worst = rows_[i].row_conflict_all;
			}
			rows_[0].row_conflict_all = worst;
			rows_[0].all_identical = (worst <= conflict_all_t::no_conflict);
		}

		endResetModel();
		return;
	}
	else if (record_type_ == "FACT")
	{
		struct fact_entry_t
		{
			std::string faction_name;
			size_t intv_idx;
			size_t anam_idx;
		};

		std::vector<std::vector<fact_entry_t>> col_entries(col_count);
		std::vector<std::string> all_faction_names;

		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
				continue;

			const auto & subs = all_sub_records[col];
			for (size_t i = 0; i + 1 < subs.size(); ++i)
			{
				if (subs[i].type != "INTV" || subs[i].size != 4)
					continue;

				if (subs[i + 1].type != "ANAM")
					continue;

				std::string faction_name(subs[i + 1].data, subs[i + 1].size);
				if (!faction_name.empty() && faction_name.back() == '\0')
					faction_name.pop_back();

				col_entries[col].push_back({ faction_name, i, i + 1 });

				bool found = false;
				for (const auto & name : all_faction_names)
				{
					if (name == faction_name)
					{
						found = true;
						break;
					}
				}

				if (!found)
					all_faction_names.push_back(faction_name);
			}
		}

		std::unordered_map<std::string, int> type_count;
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
				continue;

			for (const auto & sv : all_sub_records[col])
			{
				if (sv.type == "INTV" && sv.size == 4)
					continue;

				if (sv.type == "ANAM")
					continue;

				int occ = type_count[sv.type]++;
				bool found = false;
				for (const auto & slot : unified_slots)
				{
					if (slot.type == sv.type && slot.occurrence == occ)
					{
						found = true;
						break;
					}
				}

				if (!found)
					unified_slots.push_back({ sv.type, occ });
			}
			type_count.clear();
		}

		for (const auto & faction_name : all_faction_names)
		{
			int intv_occ = -1;
			int anam_occ = -1;

			for (const auto & slot : unified_slots)
			{
				if (slot.type == "INTV")
					intv_occ = std::max(intv_occ, slot.occurrence);

				if (slot.type == "ANAM")
					anam_occ = std::max(anam_occ, slot.occurrence);
			}

			unified_slots.push_back({ "INTV", intv_occ + 1 });
			unified_slots.push_back({ "ANAM", anam_occ + 1 });
		}

		std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
				continue;

			for (size_t i = 0; i < all_sub_records[col].size(); ++i)
			{
				const auto & sv = all_sub_records[col][i];
				if ((sv.type == "INTV" && sv.size == 4) || sv.type == "ANAM")
					continue;

				col_type_indices[col][sv.type].push_back(i);
			}

			for (size_t entry_idx = 0; entry_idx < all_faction_names.size(); ++entry_idx)
			{
				const auto & target_name = all_faction_names[entry_idx];
				bool matched = false;
				for (const auto & e : col_entries[col])
				{
					if (e.faction_name == target_name)
					{
						col_type_indices[col]["INTV"].push_back(e.intv_idx);
						col_type_indices[col]["ANAM"].push_back(e.anam_idx);
						matched = true;
						break;
					}
				}

				if (!matched)
				{
					col_type_indices[col]["INTV"].push_back(SIZE_MAX);
					col_type_indices[col]["ANAM"].push_back(SIZE_MAX);
				}
			}
		}

		for (const auto & slot : unified_slots)
		{
			sub_record_row_t row;
			row.size = 0;
			row.values.resize(col_count);

			std::string sub_type = slot.type;
			const char * first_data = nullptr;
			size_t first_size = 0;

			for (size_t col = 0; col < col_count; ++col)
			{
				if (col >= all_sub_records.size())
				{
					row.values[col] = "";
					continue;
				}

				auto & indices = col_type_indices[col][slot.type];
				if (slot.occurrence >= static_cast<int>(indices.size()))
				{
					row.values[col] = "";
					continue;
				}

				size_t idx = indices[slot.occurrence];
				if (idx == SIZE_MAX)
				{
					row.values[col] = "";
					continue;
				}

				const auto & sv = all_sub_records[col][idx];

				if (!first_data)
				{
					first_data = sv.data;
					first_size = sv.size;
					row.size = sv.size;
				}

				row.values[col] = format_value(sv.data, sv.size);
			}

			row.type = sub_type;
			row.label = make_sub_label(sub_type, record_type_, first_size);

			bool all_same = true;
			for (size_t col = 1; col < col_count; ++col)
			{
				if (row.values[col] != row.values[0])
				{
					all_same = false;
					break;
				}
			}
			row.all_identical = all_same;
			row.row_conflict_all = compute_conflict_all(row.values);
			row.cell_conflict_this = compute_conflict_this(row.values);

			const sub_record_schema_t * schema = find_schema(record_type_, sub_type, first_size);
			if (schema && first_data)
			{
				for (size_t fi = 0; fi < schema->field_count; ++fi)
				{
					const auto & fdef = schema->fields[fi];

					const bool is_flags = (fdef.type == field_type_t::flags_u8 ||
					                       fdef.type == field_type_t::flags_u16 ||
					                       fdef.type == field_type_t::flags_u32);

					if (is_flags && fdef.flag_names && fdef.flag_count > 0)
					{
						for (int bit = 0; bit < fdef.flag_count; ++bit)
						{
							if (fdef.flag_names[bit][0] == '_')
								continue;

							field_row_t frow;
							frow.name = fdef.flag_names[bit];
							frow.values.resize(col_count);

							for (size_t col = 0; col < col_count; ++col)
							{
								if (col >= all_sub_records.size())
								{
									frow.values[col] = "";
									continue;
								}

								auto & indices2 = col_type_indices[col][slot.type];
								if (slot.occurrence >= static_cast<int>(indices2.size()))
								{
									frow.values[col] = "";
									continue;
								}

								size_t idx2 = indices2[slot.occurrence];
								if (idx2 == SIZE_MAX)
								{
									frow.values[col] = "";
									continue;
								}

								const auto & sv = all_sub_records[col][idx2];
								if (fdef.offset >= sv.size)
								{
									frow.values[col] = "";
									continue;
								}

								uint32_t val = 0;
								size_t bytes = (fdef.type == field_type_t::flags_u8) ? 1 :
								               (fdef.type == field_type_t::flags_u16) ? 2 : 4;
								std::memcpy(&val, sv.data + fdef.offset, std::min(bytes, sv.size - fdef.offset));
								frow.values[col] = (val & (1u << bit)) ? "1" : "0";
							}

							bool fields_same = true;
							for (size_t col = 1; col < col_count; ++col)
							{
								if (frow.values[col] != frow.values[0])
								{
									fields_same = false;
									break;
								}
							}
							frow.all_identical = fields_same;
							frow.row_conflict_all = compute_conflict_all(frow.values);
							frow.cell_conflict_this = compute_conflict_this(frow.values);

							row.children.push_back(std::move(frow));
						}

						continue;
					}

					field_row_t frow;
					frow.name = fdef.name;
					frow.values.resize(col_count);

					for (size_t col = 0; col < col_count; ++col)
					{
						if (col >= all_sub_records.size())
						{
							frow.values[col] = "";
							continue;
						}

						auto & indices2 = col_type_indices[col][slot.type];
						if (slot.occurrence >= static_cast<int>(indices2.size()))
						{
							frow.values[col] = "";
							continue;
						}

						size_t idx2 = indices2[slot.occurrence];
						if (idx2 == SIZE_MAX)
						{
							frow.values[col] = "";
							continue;
						}

						const auto & sv = all_sub_records[col][idx2];
						frow.values[col] = decode_field(fdef, sv.data, sv.size);
					}

					bool fields_same = true;
					for (size_t col = 1; col < col_count; ++col)
					{
						if (frow.values[col] != frow.values[0])
						{
							fields_same = false;
							break;
						}
					}
					frow.all_identical = fields_same;
					frow.row_conflict_all = compute_conflict_all(frow.values);
					frow.cell_conflict_this = compute_conflict_this(frow.values);

					row.children.push_back(std::move(frow));
				}
			}
			else if (
			    first_data && first_size > 0 && !row.values.empty() && row.values[0].size() > 0 && row.values[0][0] == '<')
			{
				for (size_t offset = 0; offset < first_size; offset += 16)
				{
					field_row_t frow;
					char name_buf[16];
					std::snprintf(name_buf, sizeof(name_buf), "%04X", static_cast<unsigned>(offset));
					frow.name = name_buf;
					frow.values.resize(col_count);

					for (size_t col = 0; col < col_count; ++col)
					{
						if (col >= all_sub_records.size())
						{
							frow.values[col] = "";
							continue;
						}

						auto & indices2 = col_type_indices[col][slot.type];
						if (slot.occurrence >= static_cast<int>(indices2.size()))
						{
							frow.values[col] = "";
							continue;
						}

						size_t idx2 = indices2[slot.occurrence];
						if (idx2 == SIZE_MAX)
						{
							frow.values[col] = "";
							continue;
						}

						const auto & sv = all_sub_records[col][idx2];
						if (offset >= sv.size)
						{
							frow.values[col] = "";
							continue;
						}

						size_t chunk = std::min(static_cast<size_t>(16), sv.size - offset);
						std::string hex;
						for (size_t b = 0; b < chunk; ++b)
						{
							char hbuf[4];
							std::snprintf(hbuf, sizeof(hbuf), "%02X", static_cast<unsigned char>(sv.data[offset + b]));
							if (!hex.empty())
								hex += ' ';

							hex += hbuf;
						}
						frow.values[col] = hex;
					}

					bool fields_same = true;
					for (size_t col = 1; col < col_count; ++col)
					{
						if (frow.values[col] != frow.values[0])
						{
							fields_same = false;
							break;
						}
					}
					frow.all_identical = fields_same;
					frow.row_conflict_all = compute_conflict_all(frow.values);
					frow.cell_conflict_this = compute_conflict_this(frow.values);

					row.children.push_back(std::move(frow));
				}
			}

			rows_.push_back(std::move(row));
		}

		if (!rows_.empty())
		{
			conflict_all_t worst = conflict_all_t::only_one;
			for (size_t i = 1; i < rows_.size(); ++i)
			{
				if (rows_[i].row_conflict_all > worst)
					worst = rows_[i].row_conflict_all;
			}
			rows_[0].row_conflict_all = worst;
			rows_[0].all_identical = (worst <= conflict_all_t::no_conflict);
		}

		endResetModel();
		return;
	}
	else if (record_type_ == "CONT" || record_type_ == "CREA" || record_type_ == "NPC_")
	{
		struct cont_entry_t
		{
			std::string item_id;
			size_t npco_idx;
		};

		struct spell_entry_t
		{
			std::string spell_id;
			size_t npcs_idx;
		};

		std::vector<std::vector<cont_entry_t>> col_entries(col_count);
		std::vector<std::string> all_item_ids;
		std::vector<std::vector<spell_entry_t>> col_spells(col_count);
		std::vector<std::string> all_spell_ids;

		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
				continue;

			const auto & subs = all_sub_records[col];
			for (size_t i = 0; i < subs.size(); ++i)
			{
				if (subs[i].type == "NPCO" && subs[i].size == 36)
				{
					std::string item_id(subs[i].data + 4, 32);
					while (!item_id.empty() && item_id.back() == '\0')
						item_id.pop_back();

					col_entries[col].push_back({ item_id, i });

					bool found = false;
					for (const auto & id : all_item_ids)
					{
						if (id == item_id)
						{
							found = true;
							break;
						}
					}

					if (!found)
						all_item_ids.push_back(item_id);
				}
				else if (subs[i].type == "NPCS" && subs[i].size == 32)
				{
					std::string spell_id(subs[i].data, 32);
					while (!spell_id.empty() && spell_id.back() == '\0')
						spell_id.pop_back();

					col_spells[col].push_back({ spell_id, i });

					bool found = false;
					for (const auto & id : all_spell_ids)
					{
						if (id == spell_id)
						{
							found = true;
							break;
						}
					}

					if (!found)
						all_spell_ids.push_back(spell_id);
				}
			}
		}

		std::unordered_map<std::string, int> type_count;
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
				continue;

			for (const auto & sv : all_sub_records[col])
			{
				if (sv.type == "NPCO" && sv.size == 36)
					continue;

				if (sv.type == "NPCS" && sv.size == 32)
					continue;

				int occ = type_count[sv.type]++;
				bool found = false;
				for (const auto & slot : unified_slots)
				{
					if (slot.type == sv.type && slot.occurrence == occ)
					{
						found = true;
						break;
					}
				}

				if (!found)
					unified_slots.push_back({ sv.type, occ });
			}
			type_count.clear();
		}

		for (const auto & item_id : all_item_ids)
		{
			int npco_occ = -1;
			for (const auto & slot : unified_slots)
			{
				if (slot.type == "NPCO")
					npco_occ = std::max(npco_occ, slot.occurrence);
			}

			unified_slots.push_back({ "NPCO", npco_occ + 1 });
		}

		for (const auto & spell_id : all_spell_ids)
		{
			int npcs_occ = -1;
			for (const auto & slot : unified_slots)
			{
				if (slot.type == "NPCS")
					npcs_occ = std::max(npcs_occ, slot.occurrence);
			}

			unified_slots.push_back({ "NPCS", npcs_occ + 1 });
		}

		std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
				continue;

			for (size_t i = 0; i < all_sub_records[col].size(); ++i)
			{
				const auto & sv = all_sub_records[col][i];
				if (sv.type == "NPCO" && sv.size == 36)
					continue;

				if (sv.type == "NPCS" && sv.size == 32)
					continue;

				col_type_indices[col][sv.type].push_back(i);
			}

			for (size_t entry_idx = 0; entry_idx < all_item_ids.size(); ++entry_idx)
			{
				const auto & target_id = all_item_ids[entry_idx];
				bool matched = false;
				for (const auto & e : col_entries[col])
				{
					if (e.item_id == target_id)
					{
						col_type_indices[col]["NPCO"].push_back(e.npco_idx);
						matched = true;
						break;
					}
				}

				if (!matched)
					col_type_indices[col]["NPCO"].push_back(SIZE_MAX);
			}

			for (size_t entry_idx = 0; entry_idx < all_spell_ids.size(); ++entry_idx)
			{
				const auto & target_id = all_spell_ids[entry_idx];
				bool matched = false;
				for (const auto & e : col_spells[col])
				{
					if (e.spell_id == target_id)
					{
						col_type_indices[col]["NPCS"].push_back(e.npcs_idx);
						matched = true;
						break;
					}
				}

				if (!matched)
					col_type_indices[col]["NPCS"].push_back(SIZE_MAX);
			}
		}

		for (const auto & slot : unified_slots)
		{
			sub_record_row_t row;
			row.size = 0;
			row.values.resize(col_count);

			std::string sub_type = slot.type;
			const char * first_data = nullptr;
			size_t first_size = 0;

			for (size_t col = 0; col < col_count; ++col)
			{
				if (col >= all_sub_records.size())
				{
					row.values[col] = "";
					continue;
				}

				auto & indices = col_type_indices[col][slot.type];
				if (slot.occurrence >= static_cast<int>(indices.size()))
				{
					row.values[col] = "";
					continue;
				}

				size_t idx = indices[slot.occurrence];
				if (idx == SIZE_MAX)
				{
					row.values[col] = "";
					continue;
				}

				const auto & sv = all_sub_records[col][idx];

				if (!first_data)
				{
					first_data = sv.data;
					first_size = sv.size;
					row.size = sv.size;
				}

				row.values[col] = format_value(sv.data, sv.size);
			}

			row.type = sub_type;
			row.label = make_sub_label(sub_type, record_type_, first_size);

			bool all_same = true;
			for (size_t col = 1; col < col_count; ++col)
			{
				if (row.values[col] != row.values[0])
				{
					all_same = false;
					break;
				}
			}
			row.all_identical = all_same;
			row.row_conflict_all = compute_conflict_all(row.values);
			row.cell_conflict_this = compute_conflict_this(row.values);

			const sub_record_schema_t * schema = find_schema(record_type_, sub_type, first_size);
			if (schema && first_data)
			{
				for (size_t fi = 0; fi < schema->field_count; ++fi)
				{
					const auto & fdef = schema->fields[fi];

					const bool is_flags = (fdef.type == field_type_t::flags_u8 ||
					                       fdef.type == field_type_t::flags_u16 ||
					                       fdef.type == field_type_t::flags_u32);

					if (is_flags && fdef.flag_names && fdef.flag_count > 0)
					{
						for (int bit = 0; bit < fdef.flag_count; ++bit)
						{
							if (fdef.flag_names[bit][0] == '_')
								continue;

							field_row_t frow;
							frow.name = fdef.flag_names[bit];
							frow.values.resize(col_count);

							for (size_t col = 0; col < col_count; ++col)
							{
								if (col >= all_sub_records.size())
								{
									frow.values[col] = "";
									continue;
								}

								auto & indices2 = col_type_indices[col][slot.type];
								if (slot.occurrence >= static_cast<int>(indices2.size()))
								{
									frow.values[col] = "";
									continue;
								}

								size_t idx2 = indices2[slot.occurrence];
								if (idx2 == SIZE_MAX)
								{
									frow.values[col] = "";
									continue;
								}

								const auto & sv = all_sub_records[col][idx2];
								if (fdef.offset >= sv.size)
								{
									frow.values[col] = "";
									continue;
								}

								uint32_t val = 0;
								size_t bytes = (fdef.type == field_type_t::flags_u8) ? 1 :
								               (fdef.type == field_type_t::flags_u16) ? 2 : 4;
								std::memcpy(&val, sv.data + fdef.offset, std::min(bytes, sv.size - fdef.offset));
								frow.values[col] = (val & (1u << bit)) ? "1" : "0";
							}

							bool fields_same = true;
							for (size_t col = 1; col < col_count; ++col)
							{
								if (frow.values[col] != frow.values[0])
								{
									fields_same = false;
									break;
								}
							}
							frow.all_identical = fields_same;
							frow.row_conflict_all = compute_conflict_all(frow.values);
							frow.cell_conflict_this = compute_conflict_this(frow.values);

							row.children.push_back(std::move(frow));
						}

						continue;
					}

					field_row_t frow;
					frow.name = fdef.name;
					frow.values.resize(col_count);

					for (size_t col = 0; col < col_count; ++col)
					{
						if (col >= all_sub_records.size())
						{
							frow.values[col] = "";
							continue;
						}

						auto & indices2 = col_type_indices[col][slot.type];
						if (slot.occurrence >= static_cast<int>(indices2.size()))
						{
							frow.values[col] = "";
							continue;
						}

						size_t idx2 = indices2[slot.occurrence];
						if (idx2 == SIZE_MAX)
						{
							frow.values[col] = "";
							continue;
						}

						const auto & sv = all_sub_records[col][idx2];
						frow.values[col] = decode_field(fdef, sv.data, sv.size);
					}

					bool fields_same = true;
					for (size_t col = 1; col < col_count; ++col)
					{
						if (frow.values[col] != frow.values[0])
						{
							fields_same = false;
							break;
						}
					}
					frow.all_identical = fields_same;
					frow.row_conflict_all = compute_conflict_all(frow.values);
					frow.cell_conflict_this = compute_conflict_this(frow.values);

					row.children.push_back(std::move(frow));
				}
			}
			else if (
			    first_data && first_size > 0 && !row.values.empty() && row.values[0].size() > 0 && row.values[0][0] == '<')
			{
				for (size_t offset = 0; offset < first_size; offset += 16)
				{
					field_row_t frow;
					char name_buf[16];
					std::snprintf(name_buf, sizeof(name_buf), "%04X", static_cast<unsigned>(offset));
					frow.name = name_buf;
					frow.values.resize(col_count);

					for (size_t col = 0; col < col_count; ++col)
					{
						if (col >= all_sub_records.size())
						{
							frow.values[col] = "";
							continue;
						}

						auto & indices2 = col_type_indices[col][slot.type];
						if (slot.occurrence >= static_cast<int>(indices2.size()))
						{
							frow.values[col] = "";
							continue;
						}

						size_t idx2 = indices2[slot.occurrence];
						if (idx2 == SIZE_MAX)
						{
							frow.values[col] = "";
							continue;
						}

						const auto & sv = all_sub_records[col][idx2];
						if (offset >= sv.size)
						{
							frow.values[col] = "";
							continue;
						}

						size_t chunk = std::min(static_cast<size_t>(16), sv.size - offset);
						std::string hex;
						for (size_t b = 0; b < chunk; ++b)
						{
							char hbuf[4];
							std::snprintf(hbuf, sizeof(hbuf), "%02X", static_cast<unsigned char>(sv.data[offset + b]));
							if (!hex.empty())
								hex += ' ';

							hex += hbuf;
						}
						frow.values[col] = hex;
					}

					bool fields_same = true;
					for (size_t col = 1; col < col_count; ++col)
					{
						if (frow.values[col] != frow.values[0])
						{
							fields_same = false;
							break;
						}
					}
					frow.all_identical = fields_same;
					frow.row_conflict_all = compute_conflict_all(frow.values);
					frow.cell_conflict_this = compute_conflict_this(frow.values);

					row.children.push_back(std::move(frow));
				}
			}

			rows_.push_back(std::move(row));
		}

		if (!rows_.empty())
		{
			conflict_all_t worst = conflict_all_t::only_one;
			for (size_t i = 1; i < rows_.size(); ++i)
			{
				if (rows_[i].row_conflict_all > worst)
					worst = rows_[i].row_conflict_all;
			}
			rows_[0].row_conflict_all = worst;
			rows_[0].all_identical = (worst <= conflict_all_t::no_conflict);
		}

		endResetModel();
		return;
	}
	else
	{
		for (const auto & subs : all_sub_records)
		{
			std::unordered_map<std::string, int> type_count;
			for (const auto & sv : subs)
			{
				int occ = type_count[sv.type]++;
				bool found = false;
				for (const auto & slot : unified_slots)
				{
					if (slot.type == sv.type && slot.occurrence == occ)
					{
						found = true;
						break;
					}
				}

				if (!found)
					unified_slots.push_back({ sv.type, occ });
			}
		}
	}

	std::vector<std::unordered_map<std::string, std::vector<size_t>>> col_type_indices(col_count);
	for (size_t col = 0; col < col_count; ++col)
	{
		if (col >= all_sub_records.size())
			continue;

		for (size_t i = 0; i < all_sub_records[col].size(); ++i)
			col_type_indices[col][all_sub_records[col][i].type].push_back(i);
	}

	for (const auto & slot : unified_slots)
	{
		sub_record_row_t row;
		row.size = 0;
		row.values.resize(col_count);

		std::string sub_type = slot.type;
		const char * first_data = nullptr;
		size_t first_size = 0;

		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
			{
				row.values[col] = "";
				continue;
			}

			auto & indices = col_type_indices[col][slot.type];
			if (slot.occurrence >= static_cast<int>(indices.size()))
			{
				row.values[col] = "";
				continue;
			}

			size_t idx = indices[slot.occurrence];
			const auto & sv = all_sub_records[col][idx];

			if (!first_data)
			{
				first_data = sv.data;
				first_size = sv.size;
				row.size = sv.size;
			}

			row.values[col] = format_value(sv.data, sv.size);
		}

		row.type = sub_type;
		row.label = make_sub_label(sub_type, record_type_, first_size);

		bool all_same = true;
		for (size_t col = 1; col < col_count; ++col)
		{
			if (row.values[col] != row.values[0])
			{
				all_same = false;
				break;
			}
		}
		row.all_identical = all_same;
		row.row_conflict_all = compute_conflict_all(row.values);
		row.cell_conflict_this = compute_conflict_this(row.values);

		const sub_record_schema_t * schema = find_schema(record_type_, sub_type, first_size);
		if (schema && first_data)
		{
			for (size_t fi = 0; fi < schema->field_count; ++fi)
			{
				const auto & fdef = schema->fields[fi];

				const bool is_flags = (fdef.type == field_type_t::flags_u8 ||
				                       fdef.type == field_type_t::flags_u16 ||
				                       fdef.type == field_type_t::flags_u32);

				if (is_flags && fdef.flag_names && fdef.flag_count > 0)
				{
					for (int bit = 0; bit < fdef.flag_count; ++bit)
					{
						if (fdef.flag_names[bit][0] == '_')
							continue;

						field_row_t frow;
						frow.name = fdef.flag_names[bit];
						frow.values.resize(col_count);

						for (size_t col = 0; col < col_count; ++col)
						{
							if (col >= all_sub_records.size())
							{
								frow.values[col] = "";
								continue;
							}

							auto & indices = col_type_indices[col][slot.type];
							if (slot.occurrence >= static_cast<int>(indices.size()))
							{
								frow.values[col] = "";
								continue;
							}

							const auto & sv = all_sub_records[col][indices[slot.occurrence]];
							if (fdef.offset >= sv.size)
							{
								frow.values[col] = "";
								continue;
							}

							uint32_t val = 0;
							size_t bytes = (fdef.type == field_type_t::flags_u8) ? 1 :
							               (fdef.type == field_type_t::flags_u16) ? 2 : 4;
							std::memcpy(&val, sv.data + fdef.offset, std::min(bytes, sv.size - fdef.offset));
							frow.values[col] = (val & (1u << bit)) ? "1" : "0";
						}

						bool fields_same = true;
						for (size_t col = 1; col < col_count; ++col)
						{
							if (frow.values[col] != frow.values[0])
							{
								fields_same = false;
								break;
							}
						}
						frow.all_identical = fields_same;
						frow.row_conflict_all = compute_conflict_all(frow.values);
						frow.cell_conflict_this = compute_conflict_this(frow.values);

						row.children.push_back(std::move(frow));
					}

					continue;
				}

				field_row_t frow;
				frow.name = fdef.name;
				frow.values.resize(col_count);

				for (size_t col = 0; col < col_count; ++col)
				{
					if (col >= all_sub_records.size())
					{
						frow.values[col] = "";
						continue;
					}

					auto & indices = col_type_indices[col][slot.type];
					if (slot.occurrence >= static_cast<int>(indices.size()))
					{
						frow.values[col] = "";
						continue;
					}

					const auto & sv = all_sub_records[col][indices[slot.occurrence]];
					frow.values[col] = decode_field(fdef, sv.data, sv.size);
				}

				bool fields_same = true;
				for (size_t col = 1; col < col_count; ++col)
				{
					if (frow.values[col] != frow.values[0])
					{
						fields_same = false;
						break;
					}
				}
				frow.all_identical = fields_same;
				frow.row_conflict_all = compute_conflict_all(frow.values);
				frow.cell_conflict_this = compute_conflict_this(frow.values);

				row.children.push_back(std::move(frow));
			}
		}
		else if (
		    first_data && first_size > 0 && !row.values.empty() && row.values[0].size() > 0 && row.values[0][0] == '<')
		{
			for (size_t offset = 0; offset < first_size; offset += 16)
			{
				field_row_t frow;
				char name_buf[16];
				std::snprintf(name_buf, sizeof(name_buf), "%04X", static_cast<unsigned>(offset));
				frow.name = name_buf;
				frow.values.resize(col_count);

				for (size_t col = 0; col < col_count; ++col)
				{
					if (col >= all_sub_records.size())
					{
						frow.values[col] = "";
						continue;
					}

					auto & indices = col_type_indices[col][slot.type];
					if (slot.occurrence >= static_cast<int>(indices.size()))
					{
						frow.values[col] = "";
						continue;
					}

					const auto & sv = all_sub_records[col][indices[slot.occurrence]];
					size_t chunk = std::min(static_cast<size_t>(16), sv.size - offset);
					if (offset >= sv.size)
					{
						frow.values[col] = "";
						continue;
					}

					std::string hex;
					for (size_t b = 0; b < chunk; ++b)
					{
						char hbuf[4];
						std::snprintf(hbuf, sizeof(hbuf), "%02X", static_cast<unsigned char>(sv.data[offset + b]));
						if (!hex.empty())
							hex += ' ';

						hex += hbuf;
					}
					frow.values[col] = hex;
				}

				bool fields_same = true;
				for (size_t col = 1; col < col_count; ++col)
				{
					if (frow.values[col] != frow.values[0])
					{
						fields_same = false;
						break;
					}
				}
				frow.all_identical = fields_same;
				frow.row_conflict_all = compute_conflict_all(frow.values);
				frow.cell_conflict_this = compute_conflict_this(frow.values);

				row.children.push_back(std::move(frow));
			}
		}

		rows_.push_back(std::move(row));
	}

	if (!rows_.empty())
	{
		conflict_all_t worst = conflict_all_t::only_one;
		for (size_t i = 1; i < rows_.size(); ++i)
		{
			if (rows_[i].row_conflict_all > worst)
				worst = rows_[i].row_conflict_all;
		}
		rows_[0].row_conflict_all = worst;
		rows_[0].all_identical = (worst <= conflict_all_t::no_conflict);
	}

	endResetModel();
}

void view_tree_model_t::clear()
{
	beginResetModel();
	rows_.clear();
	column_names_.clear();
	plugin_conflict_this_.clear();
	column_plugin_indices_.clear();
	record_type_.clear();
	record_id_.clear();
	has_merge_column_ = false;
	merge_col_index_ = -1;
	filter_dirty_ = true;
	endResetModel();
}

bool view_tree_model_t::is_merge_column(int section) const
{
	if (!has_merge_column_)
		return false;

	return (section - 1) == merge_col_index_;
}

int view_tree_model_t::merge_column() const
{
	if (!has_merge_column_)
		return -1;

	return merge_col_index_ + 1;
}

void view_tree_model_t::set_hide_no_conflict(bool hide)
{
	beginResetModel();
	hide_no_conflict_ = hide;
	filter_dirty_ = true;
	endResetModel();
}

const std::vector<view_tree_model_t::sub_record_row_t> & view_tree_model_t::visible_rows() const
{
	if (!hide_no_conflict_)
		return rows_;

	if (!filter_dirty_)
		return filtered_rows_;

	filtered_rows_.clear();
	for (const auto & row : rows_)
	{
		if (!row.all_identical)
			filtered_rows_.push_back(row);
	}

	filter_dirty_ = false;
	return filtered_rows_;
}

QModelIndex view_tree_model_t::index(int row, int column, const QModelIndex & parent) const
{
	if (!hasIndex(row, column, parent))
		return {};

	if (!parent.isValid())
		return createIndex(row, column, nullptr);

	if (parent.internalPointer() != nullptr)
		return {};

	const auto & vrows = visible_rows();
	if (parent.row() < 0 || parent.row() >= static_cast<int>(vrows.size()))
		return {};

	return createIndex(row, column, const_cast<sub_record_row_t *>(&vrows[parent.row()]));
}

QModelIndex view_tree_model_t::parent(const QModelIndex & child) const
{
	if (!child.isValid())
		return {};

	auto * ptr = static_cast<sub_record_row_t *>(child.internalPointer());
	if (!ptr)
		return {};

	const auto & vrows = visible_rows();
	for (int i = 0; i < static_cast<int>(vrows.size()); ++i)
	{
		if (&vrows[i] == ptr)
			return createIndex(i, 0, nullptr);
	}

	return {};
}

int view_tree_model_t::rowCount(const QModelIndex & parent) const
{
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		return static_cast<int>(visible_rows().size());

	if (parent.internalPointer() != nullptr)
		return 0;

	const auto & vrows = visible_rows();
	if (parent.row() < 0 || parent.row() >= static_cast<int>(vrows.size()))
		return 0;

	return static_cast<int>(vrows[parent.row()].children.size());
}

int view_tree_model_t::columnCount(const QModelIndex &) const
{
	return static_cast<int>(column_names_.size()) + 1;
}

QVariant view_tree_model_t::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
		return {};

	auto * parent_ptr = static_cast<sub_record_row_t *>(index.internalPointer());

	if (!parent_ptr)
	{
		const auto & vrows = visible_rows();
		if (index.row() < 0 || index.row() >= static_cast<int>(vrows.size()))
			return {};

		const auto & row = vrows[index.row()];

		if (role == Qt::DisplayRole)
		{
			if (index.column() == 0)
				return QString::fromStdString(row.label);

			if (!row.children.empty())
				return {};

			int col = index.column() - 1;
			if (col < 0 || col >= static_cast<int>(row.values.size()))
				return {};

			return QString::fromStdString(row.values[col]);
		}

		if (role == Qt::BackgroundRole)
		{
			if (row.row_conflict_all < conflict_all_t::no_conflict)
				return {};

			if (index.column() > 0)
			{
				int col = index.column() - 1;
				if (col >= 0 && col < static_cast<int>(row.values.size()) && row.values[col].empty())
					return QBrush(lighter_hsl(conflict_all_color_raw(row.row_conflict_all), 0.93));
			}

			return QBrush(conflict_all_background(row.row_conflict_all));
		}

		if (role == Qt::ForegroundRole)
		{
			if (column_names_.size() <= 1)
				return {};

			if (index.column() == 0)
			{
				conflict_this_t worst = conflict_this_t::unknown;
				for (const auto & s : row.cell_conflict_this)
				{
					if (s > worst)
						worst = s;
				}

				return QBrush(conflict_this_foreground(worst));
			}

			int col = index.column() - 1;
			if (col < 0 || col >= static_cast<int>(row.cell_conflict_this.size()))
				return {};

			return QBrush(conflict_this_foreground(row.cell_conflict_this[col]));
		}

		return {};
	}

	if (index.row() < 0 || index.row() >= static_cast<int>(parent_ptr->children.size()))
		return {};

	const auto & frow = parent_ptr->children[index.row()];

	if (role == Qt::DisplayRole)
	{
		if (index.column() == 0)
			return QString::fromStdString(frow.name);

		int col = index.column() - 1;
		if (col < 0 || col >= static_cast<int>(frow.values.size()))
			return {};

		return QString::fromStdString(frow.values[col]);
	}

	if (role == Qt::BackgroundRole)
	{
		if (frow.row_conflict_all < conflict_all_t::no_conflict)
			return {};

		if (index.column() > 0)
		{
			int col = index.column() - 1;
			if (col >= 0 && col < static_cast<int>(frow.values.size()) && frow.values[col].empty())
				return QBrush(lighter_hsl(conflict_all_color_raw(frow.row_conflict_all), 0.93));
		}

		return QBrush(conflict_all_background(frow.row_conflict_all));
	}

	if (role == Qt::ForegroundRole)
	{
		if (column_names_.size() <= 1)
			return {};

		if (index.column() == 0)
		{
			conflict_this_t worst = conflict_this_t::unknown;
			for (const auto & s : frow.cell_conflict_this)
			{
				if (s > worst)
					worst = s;
			}

			return QBrush(conflict_this_foreground(worst));
		}

		int col = index.column() - 1;
		if (col < 0 || col >= static_cast<int>(frow.cell_conflict_this.size()))
			return {};

		return QBrush(conflict_this_foreground(frow.cell_conflict_this[col]));
	}

	return {};
}

QVariant view_tree_model_t::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return {};

	if (section == 0)
		return QStringLiteral("Sub-Record");

	int col = section - 1;
	if (col < 0 || col >= static_cast<int>(column_names_.size()))
		return {};

	return QString::fromStdString(column_names_[col]);
}

Qt::ItemFlags view_tree_model_t::flags(const QModelIndex & index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	if (index.column() > 0 && index.column() <= static_cast<int>(column_plugin_indices_.size()))
	{
		int col = index.column() - 1;
		if (!is_merge_column(index.column()))
			f |= Qt::ItemIsDragEnabled;

		if (is_merge_column(index.column()))
			f |= Qt::ItemIsDropEnabled;
	}

	return f;
}

Qt::DropActions view_tree_model_t::supportedDragActions() const
{
	return Qt::CopyAction;
}

QMimeData * view_tree_model_t::mimeData(const QModelIndexList & indexes) const
{
	if (indexes.isEmpty())
		return nullptr;

	const auto & idx = indexes.first();
	if (idx.column() <= 0)
		return nullptr;

	int col = idx.column() - 1;
	if (col < 0 || col >= static_cast<int>(column_plugin_indices_.size()))
		return nullptr;

	if (is_merge_column(idx.column()))
		return nullptr;

	int plugin_idx = column_plugin_indices_[col];
	auto * mime = new QMimeData;
	QString payload = QString("%1\t%2\t%3")
	                      .arg(plugin_idx)
	                      .arg(QString::fromStdString(record_type_))
	                      .arg(QString::fromStdString(record_id_));
	mime->setData("application/x-yampt-record", payload.toUtf8());
	return mime;
}

Qt::DropActions view_tree_model_t::supportedDropActions() const
{
	return Qt::CopyAction;
}

bool view_tree_model_t::canDropMimeData(const QMimeData * data, Qt::DropAction, int, int, const QModelIndex &) const
{
	if (!has_merge_column_)
		return false;

	return data && data->hasFormat("application/x-yampt-record");
}
