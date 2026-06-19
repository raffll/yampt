#include "view_tree_model.hpp"
#include <QBrush>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <map>

static const std::map<std::string, const char *> & sub_record_descriptions()
{
	static const std::map<std::string, const char *> descs = {
		{"NAME", "ID"},
		{"FNAM", "Name"},
		{"MODL", "Model Filename"},
		{"SCRI", "Script"},
		{"ITEX", "Icon"},
		{"ENAM", "Enchantment Effect"},
		{"ANAM", "Faction/Owner"},
		{"BNAM", "Script Text"},
		{"CNAM", "Class"},
		{"DNAM", "Destination"},
		{"ONAM", "Actor"},
		{"RNAM", "Race"},
		{"INDX", "Index"},
		{"INTV", "Integer Value"},
		{"FLTV", "Float Value"},
		{"STRV", "String Value"},
		{"INAM", "Info ID"},
		{"PNAM", "Previous Info"},
		{"NNAM", "Next Info"},
		{"SNAM", "Sound"},
		{"DATA", "Data"},
		{"FLAG", "Flags"},
		{"NPDT", "NPC Data"},
		{"AIDT", "AI Data"},
		{"WPDT", "Weapon Data"},
		{"AODT", "Armor Data"},
		{"ALDT", "Potion Data"},
		{"ENDT", "Enchantment Data"},
		{"BKDT", "Book Data"},
		{"CNDT", "Container Data"},
		{"FADT", "Faction Data"},
		{"CLDT", "Class Data"},
		{"RADT", "Race Data"},
		{"SPDT", "Spell Data"},
		{"WEAT", "Weather"},
		{"WHGT", "Water Height"},
		{"AMBI", "Ambient Light"},
		{"RGNN", "Region Name"},
		{"DELE", "Deleted"},
		{"SCVR", "Script Variable"},
		{"SCHD", "Script Header"},
		{"SCTX", "Script Source"},
		{"SCDT", "Script Data"},
		{"HEDR", "Header"},
		{"MAST", "Master File"},
		{"DODT", "Door Destination"},
		{"FRMR", "Object Reference"},
		{"XSCL", "Scale"},
		{"NAM0", "Object Count"},
		{"NAM5", "Map Color"},
	};
	return descs;
}

view_tree_model_t::view_tree_model_t(QObject * parent)
	: QAbstractItemModel(parent)
{
}

void view_tree_model_t::set_record(plugin_scan_t & scan, const conflict_entry_t & entry)
{
	beginResetModel();

	rows_.clear();
	column_names_.clear();
	plugin_conflict_this_.clear();
	record_type_ = entry.rec_type;
	filter_dirty_ = true;

	size_t col_count = entry.versions.size();

	for (const auto & ver : entry.versions)
	{
		char buf[64];
		std::snprintf(buf, sizeof(buf), "[%02X] %s", ver.plugin_idx,
			scan.plugin_filename(ver.plugin_idx).c_str());
		column_names_.push_back(buf);
		plugin_conflict_this_.push_back(ver.status);
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
		header_row.row_conflict_all = entry.conflict_all;
		header_row.all_identical = (entry.conflict_all == conflict_all_t::no_conflict
		                         || entry.conflict_all == conflict_all_t::only_one);
		header_row.cell_conflict_this.resize(col_count, conflict_this_t::master);

		field_row_t sig_row;
		sig_row.name = "Signature";
		sig_row.values.resize(col_count, entry.rec_type);
		sig_row.row_conflict_all = conflict_all_t::no_conflict;
		sig_row.all_identical = true;
		sig_row.cell_conflict_this = compute_row_conflict_this(sig_row.values);
		header_row.children.push_back(std::move(sig_row));

		field_row_t flags_row;
		flags_row.name = "Record Flags";
		flags_row.values.resize(col_count);

		for (size_t col = 0; col < col_count; ++col)
		{
			const auto & records = scan.plugin(entry.versions[col].plugin_idx).get_records();
			if (entry.versions[col].record_index >= records.size())
				continue;

			const auto & content = records[entry.versions[col].record_index].content;
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
		flags_row.row_conflict_all = compute_row_conflict_all(flags_row.values);
		flags_row.cell_conflict_this = compute_row_conflict_this(flags_row.values);
		header_row.children.push_back(std::move(flags_row));

		rows_.push_back(std::move(header_row));
	}

	std::vector<std::vector<sub_record_view_t>> all_sub_records;

	for (const auto & ver : entry.versions)
	{
		const auto & records = scan.plugin(ver.plugin_idx).get_records();
		if (ver.record_index >= records.size())
		{
			all_sub_records.push_back({});
			continue;
		}

		const auto & content = records[ver.record_index].content;
		if (content.size() < 16)
		{
			all_sub_records.push_back({});
			continue;
		}

		std::vector<sub_record_view_t> subs;
		sub_record_iter_t iter(content);
		sub_record_view_t sv;
		while (iter.next(sv))
			subs.push_back(sv);

		all_sub_records.push_back(std::move(subs));
	}

	size_t max_count = 0;
	for (const auto & subs : all_sub_records)
	{
		if (subs.size() > max_count)
			max_count = subs.size();
	}

	for (size_t row_idx = 0; row_idx < max_count; ++row_idx)
	{
		sub_record_row_t row;
		row.size = 0;
		row.values.resize(col_count);

		std::string sub_type;
		const char * first_data = nullptr;
		size_t first_size = 0;

		for (size_t col = 0; col < col_count; ++col)
		{
			if (row_idx >= all_sub_records[col].size())
			{
				row.values[col] = "";
				continue;
			}

			const auto & sv = all_sub_records[col][row_idx];

			if (sub_type.empty())
			{
				sub_type = sv.type;
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
		row.row_conflict_all = compute_row_conflict_all(row.values);
		row.cell_conflict_this = compute_row_conflict_this(row.values);

		const sub_record_schema_t * schema = find_schema(record_type_, sub_type, first_size);
		if (schema && first_data)
		{
			for (size_t fi = 0; fi < schema->field_count; ++fi)
			{
				const auto & fdef = schema->fields[fi];

				field_row_t frow;
				frow.name = fdef.name;
				frow.values.resize(col_count);

				for (size_t col = 0; col < col_count; ++col)
				{
					if (row_idx >= all_sub_records[col].size())
					{
						frow.values[col] = "";
						continue;
					}

					const auto & sv = all_sub_records[col][row_idx];
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
				frow.row_conflict_all = compute_row_conflict_all(frow.values);
				frow.cell_conflict_this = compute_row_conflict_this(frow.values);

				row.children.push_back(std::move(frow));
			}
		}

		rows_.push_back(std::move(row));
	}

	endResetModel();
}

void view_tree_model_t::clear()
{
	beginResetModel();
	rows_.clear();
	column_names_.clear();
	plugin_conflict_this_.clear();
	record_type_.clear();
	filter_dirty_ = true;
	endResetModel();
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

std::string view_tree_model_t::format_value(const char * data, size_t size) const
{
	if (size > 256)
	{
		char buf[64];
		std::snprintf(buf, sizeof(buf), "<%zu bytes>", size);
		return std::string(buf);
	}

	bool printable = true;
	for (size_t i = 0; i < size; ++i)
	{
		unsigned char c = static_cast<unsigned char>(data[i]);
		if (c == 0)
			continue;

		if (c < 32 || c > 126)
		{
			printable = false;
			break;
		}
	}

	if (printable)
	{
		size_t len = 0;
		for (size_t i = 0; i < size; ++i)
		{
			if (data[i] == '\0')
				break;

			++len;
		}
		return std::string(data, len);
	}

	std::string hex;
	size_t limit = std::min(size, static_cast<size_t>(64));
	for (size_t i = 0; i < limit; ++i)
	{
		char buf[4];
		std::snprintf(buf, sizeof(buf), "%02X", static_cast<unsigned char>(data[i]));
		if (!hex.empty())
			hex += ' ';

		hex += buf;
	}

	if (size > 64)
		hex += " ...";

	return hex;
}

std::string view_tree_model_t::decode_field(const field_def_t & field, const char * data, size_t data_size) const
{
	if (field.offset + field.size > data_size && field.type != field_type_t::string_var)
		return "";

	const char * ptr = data + field.offset;
	char buf[128];

	switch (field.type)
	{
	case field_type_t::u8:
	{
		uint8_t val = 0;
		std::memcpy(&val, ptr, 1);
		std::snprintf(buf, sizeof(buf), "%u", val);
		return buf;
	}
	case field_type_t::u16:
	{
		uint16_t val = 0;
		std::memcpy(&val, ptr, 2);
		std::snprintf(buf, sizeof(buf), "%u", val);
		return buf;
	}
	case field_type_t::u32:
	{
		uint32_t val = 0;
		std::memcpy(&val, ptr, 4);
		std::snprintf(buf, sizeof(buf), "%u", val);
		return buf;
	}
	case field_type_t::i8:
	{
		int8_t val = 0;
		std::memcpy(&val, ptr, 1);
		std::snprintf(buf, sizeof(buf), "%d", val);
		return buf;
	}
	case field_type_t::i16:
	{
		int16_t val = 0;
		std::memcpy(&val, ptr, 2);
		std::snprintf(buf, sizeof(buf), "%d", val);
		return buf;
	}
	case field_type_t::i32:
	{
		int32_t val = 0;
		std::memcpy(&val, ptr, 4);
		std::snprintf(buf, sizeof(buf), "%d", val);
		return buf;
	}
	case field_type_t::f32:
	{
		float val = 0;
		std::memcpy(&val, ptr, 4);
		std::snprintf(buf, sizeof(buf), "%.4f", val);
		return buf;
	}
	case field_type_t::string_fixed:
	{
		size_t len = 0;
		for (size_t i = 0; i < field.size && field.offset + i < data_size; ++i)
		{
			if (ptr[i] == '\0')
				break;

			++len;
		}
		return std::string(ptr, len);
	}
	case field_type_t::string_var:
	{
		if (field.offset >= data_size)
			return "";

		size_t remaining = data_size - field.offset;
		size_t len = 0;
		for (size_t i = 0; i < remaining; ++i)
		{
			if (ptr[i] == '\0')
				break;

			++len;
		}
		return std::string(ptr, len);
	}
	case field_type_t::flags_u8:
	{
		uint8_t val = 0;
		std::memcpy(&val, ptr, 1);
		std::snprintf(buf, sizeof(buf), "0x%X", val);
		std::string result = buf;

		if (field.flag_names)
		{
			std::string names;
			for (int bit = 0; bit < 8; ++bit)
			{
				if (!(val & (1u << bit)))
					continue;

				if (!field.flag_names[bit])
					continue;

				if (!names.empty())
					names += " | ";

				names += field.flag_names[bit];
			}

			if (!names.empty())
				result += " (" + names + ")";
		}

		return result;
	}
	case field_type_t::flags_u16:
	{
		uint16_t val = 0;
		std::memcpy(&val, ptr, 2);
		std::snprintf(buf, sizeof(buf), "0x%X", val);
		std::string result = buf;

		if (field.flag_names)
		{
			std::string names;
			for (int bit = 0; bit < 16; ++bit)
			{
				if (!(val & (1u << bit)))
					continue;

				if (!field.flag_names[bit])
					continue;

				if (!names.empty())
					names += " | ";

				names += field.flag_names[bit];
			}

			if (!names.empty())
				result += " (" + names + ")";
		}

		return result;
	}
	case field_type_t::flags_u32:
	{
		uint32_t val = 0;
		std::memcpy(&val, ptr, 4);
		std::snprintf(buf, sizeof(buf), "0x%X", val);
		std::string result = buf;

		if (field.flag_names)
		{
			std::string names;
			for (int bit = 0; bit < 32; ++bit)
			{
				if (!(val & (1u << bit)))
					continue;

				if (!field.flag_names[bit])
					continue;

				if (!names.empty())
					names += " | ";

				names += field.flag_names[bit];
			}

			if (!names.empty())
				result += " (" + names + ")";
		}

		return result;
	}
	case field_type_t::enum_u8:
	{
		uint8_t val = 0;
		std::memcpy(&val, ptr, 1);
		std::snprintf(buf, sizeof(buf), "%u", val);
		std::string result = buf;

		if (field.enum_names && field.enum_names[val])
			result += " (" + std::string(field.enum_names[val]) + ")";

		return result;
	}
	case field_type_t::enum_u16:
	{
		uint16_t val = 0;
		std::memcpy(&val, ptr, 2);
		std::snprintf(buf, sizeof(buf), "%u", val);
		std::string result = buf;

		if (field.enum_names && field.enum_names[val])
			result += " (" + std::string(field.enum_names[val]) + ")";

		return result;
	}
	case field_type_t::enum_u32:
	{
		uint32_t val = 0;
		std::memcpy(&val, ptr, 4);
		std::snprintf(buf, sizeof(buf), "%u", val);
		std::string result = buf;

		if (field.enum_names && field.enum_names[val])
			result += " (" + std::string(field.enum_names[val]) + ")";

		return result;
	}
	case field_type_t::raw:
	{
		std::string hex;
		size_t limit = std::min(field.size, static_cast<size_t>(64));
		for (size_t i = 0; i < limit && field.offset + i < data_size; ++i)
		{
			char hbuf[4];
			std::snprintf(hbuf, sizeof(hbuf), "%02X", static_cast<unsigned char>(ptr[i]));
			if (!hex.empty())
				hex += ' ';

			hex += hbuf;
		}

		if (field.size > 64)
			hex += " ...";

		return hex;
	}
	}

	return "";
}

std::string view_tree_model_t::make_sub_label(const std::string & sub_type,
                                               const std::string & record_type,
                                               size_t data_size) const
{
	const auto & descs = sub_record_descriptions();
	auto it = descs.find(sub_type);

	const sub_record_schema_t * schema = find_schema(record_type, sub_type, data_size);

	if (schema)
	{
		std::string parent_name;
		const char * dn = nullptr;

		static const std::map<std::string, const char *> record_names = {
			{"ACTI", "Activator"}, {"ALCH", "Potion"}, {"ARMO", "Armor"},
			{"BOOK", "Book"}, {"CELL", "Cell"}, {"CLAS", "Class"},
			{"CLOT", "Clothing"}, {"CONT", "Container"}, {"CREA", "Creature"},
			{"ENCH", "Enchantment"}, {"FACT", "Faction"}, {"INFO", "Info"},
			{"LEVI", "Leveled Item"}, {"LEVC", "Leveled Creature"},
			{"NPC_", "NPC"}, {"RACE", "Race"}, {"REGN", "Region"},
			{"SPEL", "Spell"}, {"WEAP", "Weapon"},
		};

		auto rit = record_names.find(record_type);
		if (rit != record_names.end())
			parent_name = rit->second;
		else
			parent_name = record_type;

		if (it != descs.end())
			return sub_type + " - " + parent_name + " " + it->second;

		return sub_type + " - " + parent_name + " Data";
	}

	if (it != descs.end())
		return sub_type + " - " + it->second;

	return sub_type;
}

conflict_all_t view_tree_model_t::compute_row_conflict_all(
	const std::vector<std::string> & values) const
{
	if (values.size() <= 1)
		return conflict_all_t::only_one;

	bool all_same = true;
	for (size_t i = 1; i < values.size(); ++i)
	{
		if (values[i] != values[0])
		{
			all_same = false;
			break;
		}
	}

	if (all_same)
		return conflict_all_t::no_conflict;

	if (values.size() == 2)
		return conflict_all_t::override_benign;

	bool only_last_differs = true;
	for (size_t i = 1; i < values.size() - 1; ++i)
	{
		if (values[i] != values[0])
		{
			only_last_differs = false;
			break;
		}
	}

	if (only_last_differs)
		return conflict_all_t::override_benign;

	return conflict_all_t::conflict;
}

std::vector<conflict_this_t> view_tree_model_t::compute_row_conflict_this(
	const std::vector<std::string> & values) const
{
	std::vector<conflict_this_t> result(values.size(), conflict_this_t::unknown);

	if (values.empty())
		return result;

	if (values.size() == 1)
	{
		result[0] = values[0].empty() ? conflict_this_t::unknown : conflict_this_t::master;
		return result;
	}

	result[0] = values[0].empty() ? conflict_this_t::unknown : conflict_this_t::master;

	size_t last_idx = values.size() - 1;

	for (size_t i = 1; i < values.size(); ++i)
	{
		if (values[i].empty() && values[0].empty())
		{
			result[i] = conflict_this_t::identical_to_master;
			continue;
		}

		if (values[i].empty())
		{
			result[i] = conflict_this_t::unknown;
			continue;
		}

		if (values[i] == values[0])
		{
			result[i] = conflict_this_t::identical_to_master;
			continue;
		}

		if (i == last_idx)
			result[i] = conflict_this_t::override_wins;
		else
			result[i] = conflict_this_t::conflict_loses;
	}

	bool any_differs = false;
	int differ_count = 0;
	for (size_t i = 1; i < values.size(); ++i)
	{
		if (result[i] != conflict_this_t::identical_to_master && result[i] != conflict_this_t::unknown)
		{
			any_differs = true;
			++differ_count;
		}
	}

	if (!any_differs)
		return result;

	if (differ_count > 1)
	{
		for (size_t i = 1; i < values.size(); ++i)
		{
			if (result[i] == conflict_this_t::override_wins)
				result[i] = conflict_this_t::conflict_wins;
		}
	}

	return result;
}
