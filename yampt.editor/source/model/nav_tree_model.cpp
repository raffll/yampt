#include "nav_tree_model.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <map>
#include <QBrush>

static int natural_compare(const std::string & a, const std::string & b)
{
	size_t i = 0, j = 0;
	while (i < a.size() && j < b.size())
	{
		bool a_digit = std::isdigit(static_cast<unsigned char>(a[i]));
		bool b_digit = std::isdigit(static_cast<unsigned char>(b[j]));

		if (a_digit && b_digit)
		{
			while (i < a.size() && a[i] == '0')
				++i;
			while (j < b.size() && b[j] == '0')
				++j;

			size_t a_start = i, b_start = j;
			while (i < a.size() && std::isdigit(static_cast<unsigned char>(a[i])))
				++i;
			while (j < b.size() && std::isdigit(static_cast<unsigned char>(b[j])))
				++j;

			size_t a_len = i - a_start;
			size_t b_len = j - b_start;

			if (a_len != b_len)
				return a_len < b_len ? -1 : 1;

			for (size_t k = 0; k < a_len; ++k)
			{
				if (a[a_start + k] != b[b_start + k])
					return a[a_start + k] < b[b_start + k] ? -1 : 1;
			}
		}
		else
		{
			unsigned char ca = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(a[i])));
			unsigned char cb = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(b[j])));

			if (ca != cb)
				return ca < cb ? -1 : 1;

			++i;
			++j;
		}
	}

	if (i < a.size())
		return 1;
	if (j < b.size())
		return -1;
	return 0;
}

static const char * type_to_display_name(const std::string & type)
{
	static const std::map<std::string, const char *> names = {
		{ "ACTI", "Activator" },
		{ "ALCH", "Potion" },
		{ "APPA", "Apparatus" },
		{ "ARMO", "Armor" },
		{ "BODY", "Body Part" },
		{ "BOOK", "Book" },
		{ "BSGN", "Birthsign" },
		{ "CELL", "Cell" },
		{ "CLAS", "Class" },
		{ "CLOT", "Clothing" },
		{ "CONT", "Container" },
		{ "CREA", "Creature" },
		{ "DIAL", "Dialogue" },
		{ "DOOR", "Door" },
		{ "ENCH", "Enchantment" },
		{ "FACT", "Faction" },
		{ "GLOB", "Global" },
		{ "GMST", "Game Setting" },
		{ "INFO", "Dialogue Response" },
		{ "INGR", "Ingredient" },
		{ "LAND", "Landscape" },
		{ "LEVC", "Leveled Creature" },
		{ "LEVI", "Leveled Item" },
		{ "LIGH", "Light" },
		{ "LOCK", "Lockpick" },
		{ "LTEX", "Land Texture" },
		{ "MGEF", "Magic Effect" },
		{ "MISC", "Misc. Item" },
		{ "NPC_", "NPC" },
		{ "PGRD", "Path Grid" },
		{ "PROB", "Probe" },
		{ "RACE", "Race" },
		{ "REGN", "Region" },
		{ "REPA", "Repair Item" },
		{ "SCPT", "Script" },
		{ "SKIL", "Skill" },
		{ "SNDG", "Sound Generator" },
		{ "SOUN", "Sound" },
		{ "SPEL", "Spell" },
		{ "SSCR", "Start Script" },
		{ "STAT", "Static" },
		{ "TES3", "File Header" },
		{ "WEAP", "Weapon" },
	};

	auto it = names.find(type);
	if (it != names.end())
		return it->second;

	return nullptr;
}

nav_tree_model_t::nav_tree_model_t(plugin_scan_t & scan, QObject * parent)
    : QAbstractItemModel(parent)
    , scan_(scan)
{}

void nav_tree_model_t::rebuild()
{
	beginResetModel();
	build_tree();
	endResetModel();
}

void nav_tree_model_t::set_filter(const filter_state_t & state)
{
	has_filter_ = true;
	filter_ = state;
	rebuild();
}

void nav_tree_model_t::clear_filter()
{
	has_filter_ = false;
	filter_ = {};
	rebuild();
}

void nav_tree_model_t::build_tree()
{
	tree_.clear();

	const auto & entries = scan_.entries();

	for (int p = 0; p < static_cast<int>(scan_.plugin_count()); ++p)
	{
		file_node_t file_node;
		file_node.plugin_idx = p;

		std::map<std::string, std::vector<visible_record_t>> type_map;

		for (size_t ei = 0; ei < entries.size(); ++ei)
		{
			const auto & entry = entries[ei];

			bool has_version = false;
			for (const auto & v : entry.versions)
			{
				if (v.plugin_idx == p)
				{
					has_version = true;
					break;
				}
			}

			if (!has_version)
				continue;

			if (has_filter_ && !passes_filter(entry, p))
				continue;

			type_map[entry.rec_type].push_back({ ei });
		}

		for (auto & [type, recs] : type_map)
		{
			std::sort(
			    recs.begin(),
			    recs.end(),
			    [&](const visible_record_t & a, const visible_record_t & b)
			{
				const auto & ea = entries[a.entry_idx];
				const auto & eb = entries[b.entry_idx];
				const auto & na = ea.display_name;
				const auto & nb = eb.display_name;

				if (!na.empty() && !nb.empty())
					return natural_compare(na, nb) < 0;

				if (na.empty() && nb.empty())
					return natural_compare(ea.record_id, eb.record_id) < 0;

				return !na.empty();
			});

			type_group_t group;
			group.type = type;
			group.records = std::move(recs);
			file_node.groups.push_back(std::move(group));
		}

		if (file_node.groups.empty())
			continue;

		std::sort(
		    file_node.groups.begin(),
		    file_node.groups.end(),
		    [](const type_group_t & a, const type_group_t & b)
		{
			const char * na = type_to_display_name(a.type);
			const char * nb = type_to_display_name(b.type);
			const char * sa = na ? na : a.type.c_str();
			const char * sb = nb ? nb : b.type.c_str();
			return std::strcmp(sa, sb) < 0;
		});

		tree_.push_back(std::move(file_node));
	}
}

bool nav_tree_model_t::passes_filter(const conflict_entry_t & entry, int plugin_idx) const
{
	if (filter_.filter_conflict_all && !filter_.conflict_all_set.empty())
	{
		if (filter_.conflict_all_set.find(entry.conflict_all) == filter_.conflict_all_set.end())
			return false;
	}

	if (filter_.filter_conflict_this && !filter_.conflict_this_set.empty())
	{
		bool found = false;
		for (const auto & v : entry.versions)
		{
			if (v.plugin_idx == plugin_idx &&
			    filter_.conflict_this_set.find(v.status) != filter_.conflict_this_set.end())
			{
				found = true;
				break;
			}
		}

		if (!found)
			return false;
	}

	if (filter_.filter_by_type && !filter_.type_set.empty())
	{
		if (filter_.type_set.find(entry.rec_type) == filter_.type_set.end())
			return false;
	}

	if (filter_.filter_by_id && !filter_.id_text.empty())
	{
		auto lower_id = entry.record_id;
		auto lower_search = filter_.id_text;
		std::transform(
		    lower_id.begin(),
		    lower_id.end(),
		    lower_id.begin(),
		    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		std::transform(
		    lower_search.begin(),
		    lower_search.end(),
		    lower_search.begin(),
		    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (lower_id.find(lower_search) == std::string::npos)
			return false;
	}

	if (filter_.filter_by_name && !filter_.name_text.empty())
	{
		auto lower_name = entry.display_name;
		auto lower_search = filter_.name_text;
		std::transform(
		    lower_name.begin(),
		    lower_name.end(),
		    lower_name.begin(),
		    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		std::transform(
		    lower_search.begin(),
		    lower_search.end(),
		    lower_search.begin(),
		    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (lower_name.find(lower_search) == std::string::npos)
			return false;
	}

	if (filter_.filter_deleted)
	{
		bool has_deleted = false;
		for (const auto & v : entry.versions)
		{
			if (v.plugin_idx == plugin_idx && v.status == conflict_this_t::deleted)
			{
				has_deleted = true;
				break;
			}
		}

		if (!has_deleted)
			return false;
	}

	if (filter_.filter_itm_only)
	{
		bool has_itm = false;
		for (const auto & v : entry.versions)
		{
			if (v.plugin_idx == plugin_idx && v.status == conflict_this_t::identical_to_master)
			{
				has_itm = true;
				break;
			}
		}

		if (!has_itm)
			return false;
	}

	return true;
}

QModelIndex nav_tree_model_t::index(int row, int column, const QModelIndex & parent) const
{
	if (column < 0 || column >= 3)
		return {};

	if (!parent.isValid())
	{
		if (row < 0 || row >= static_cast<int>(tree_.size()))
			return {};

		return createIndex(row, column, nullptr);
	}

	void * ptr = parent.internalPointer();

	if (ptr == nullptr)
	{
		int file_idx = parent.row();
		if (file_idx < 0 || file_idx >= static_cast<int>(tree_.size()))
			return {};

		const auto & file_node = tree_[static_cast<size_t>(file_idx)];
		if (row < 0 || row >= static_cast<int>(file_node.groups.size()))
			return {};

		return createIndex(row, column, const_cast<file_node_t *>(&file_node));
	}

	const auto * file_ptr = static_cast<const file_node_t *>(ptr);
	for (size_t fi = 0; fi < tree_.size(); ++fi)
	{
		if (&tree_[fi] == file_ptr)
		{
			int group_idx = parent.row();
			if (group_idx < 0 || group_idx >= static_cast<int>(file_ptr->groups.size()))
				return {};

			const auto & group = file_ptr->groups[static_cast<size_t>(group_idx)];
			if (row < 0 || row >= static_cast<int>(group.records.size()))
				return {};

			return createIndex(row, column, const_cast<type_group_t *>(&group));
		}
	}

	return {};
}

QModelIndex nav_tree_model_t::parent(const QModelIndex & child) const
{
	if (!child.isValid())
		return {};

	void * ptr = child.internalPointer();

	if (ptr == nullptr)
		return {};

	for (size_t fi = 0; fi < tree_.size(); ++fi)
	{
		if (ptr == &tree_[fi])
			return createIndex(static_cast<int>(fi), 0, nullptr);

		for (size_t gi = 0; gi < tree_[fi].groups.size(); ++gi)
		{
			if (ptr == &tree_[fi].groups[gi])
				return createIndex(static_cast<int>(gi), 0, const_cast<file_node_t *>(&tree_[fi]));
		}
	}

	return {};
}

int nav_tree_model_t::rowCount(const QModelIndex & parent) const
{
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		return static_cast<int>(tree_.size());

	void * ptr = parent.internalPointer();

	if (ptr == nullptr)
	{
		int file_idx = parent.row();
		if (file_idx < 0 || file_idx >= static_cast<int>(tree_.size()))
			return 0;

		return static_cast<int>(tree_[static_cast<size_t>(file_idx)].groups.size());
	}

	for (size_t fi = 0; fi < tree_.size(); ++fi)
	{
		if (ptr == &tree_[fi])
		{
			int group_idx = parent.row();
			if (group_idx < 0 || group_idx >= static_cast<int>(tree_[fi].groups.size()))
				return 0;

			return static_cast<int>(tree_[fi].groups[static_cast<size_t>(group_idx)].records.size());
		}
	}

	return 0;
}

int nav_tree_model_t::columnCount(const QModelIndex &) const
{
	return 2;
}

QVariant nav_tree_model_t::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return {};

	switch (section)
	{
	case 0:
		return QStringLiteral("EditorID");
	case 1:
		return QStringLiteral("Name");
	}

	return {};
}

QVariant nav_tree_model_t::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
		return {};

	void * ptr = index.internalPointer();
	const auto & entries = scan_.entries();
	int col = index.column();

	if (ptr == nullptr)
	{
		int file_idx = index.row();
		if (file_idx < 0 || file_idx >= static_cast<int>(tree_.size()))
			return {};

		const auto & file_node = tree_[static_cast<size_t>(file_idx)];

		if (role == Qt::DisplayRole && col == 0)
		{
			char buf[64];
			std::snprintf(
			    buf,
			    sizeof(buf),
			    "[%02X] %s",
			    file_node.plugin_idx,
			    scan_.plugin_filename(file_node.plugin_idx).c_str());
			return QString::fromUtf8(buf);
		}

		if (role == Qt::BackgroundRole)
		{
			conflict_all_t worst = conflict_all_t::only_one;
			for (const auto & group : file_node.groups)
			{
				for (const auto & rec : group.records)
				{
					const auto & e = entries[rec.entry_idx];
					if (e.conflict_all > worst)
						worst = e.conflict_all;
				}
			}

			if (worst < conflict_all_t::no_conflict)
				return {};

			return QBrush(conflict_all_background(worst));
		}

		if (role == Qt::ForegroundRole)
		{
			conflict_this_t worst_ct = conflict_this_t::unknown;
			for (const auto & group : file_node.groups)
			{
				for (const auto & rec : group.records)
				{
					const auto & e = entries[rec.entry_idx];
					for (const auto & v : e.versions)
					{
						if (v.plugin_idx != file_node.plugin_idx)
							continue;

						if (v.status > worst_ct)
							worst_ct = v.status;
					}
				}
			}

			return QBrush(conflict_this_foreground(worst_ct));
		}

		if (role == Qt::ToolTipRole)
		{
			size_t itm = scan_.itm_count(file_node.plugin_idx);
			if (itm > 0)
				return QString("%1 ITM records").arg(itm);

			return {};
		}

		return {};
	}

	for (size_t fi = 0; fi < tree_.size(); ++fi)
	{
		if (ptr == &tree_[fi])
		{
			int group_idx = index.row();
			if (group_idx < 0 || group_idx >= static_cast<int>(tree_[fi].groups.size()))
				return {};

			const auto & group = tree_[fi].groups[static_cast<size_t>(group_idx)];

			if (role == Qt::DisplayRole && col == 0)
			{
				const char * display_name = type_to_display_name(group.type);
				if (display_name)
					return QString("%1 [%2]").arg(display_name).arg(group.records.size());

				return QString("%1 (%2)").arg(QString::fromStdString(group.type)).arg(group.records.size());
			}

			if (role == Qt::BackgroundRole)
			{
				conflict_all_t worst = conflict_all_t::only_one;
				for (const auto & rec : group.records)
				{
					const auto & e = entries[rec.entry_idx];
					if (e.conflict_all > worst)
						worst = e.conflict_all;
				}

				if (worst < conflict_all_t::no_conflict)
					return {};

				return QBrush(conflict_all_background(worst));
			}

			if (role == Qt::ForegroundRole)
			{
				conflict_this_t worst_ct = conflict_this_t::unknown;
				for (const auto & rec : group.records)
				{
					const auto & e = entries[rec.entry_idx];
					for (const auto & v : e.versions)
					{
						if (v.plugin_idx != tree_[fi].plugin_idx)
							continue;

						if (v.status > worst_ct)
							worst_ct = v.status;
					}
				}

				return QBrush(conflict_this_foreground(worst_ct));
			}

			return {};
		}

		for (size_t gi = 0; gi < tree_[fi].groups.size(); ++gi)
		{
			if (ptr != &tree_[fi].groups[gi])
				continue;

			int rec_idx = index.row();
			if (rec_idx < 0 || rec_idx >= static_cast<int>(tree_[fi].groups[gi].records.size()))
				return {};

			const auto & vis = tree_[fi].groups[gi].records[static_cast<size_t>(rec_idx)];
			const auto & entry = entries[vis.entry_idx];

			if (role == Qt::DisplayRole)
			{
				if (col == 0)
					return QString::fromStdString(entry.record_id);

				if (col == 1)
					return QString::fromStdString(entry.display_name);
			}

			if (role == Qt::BackgroundRole)
			{
				if (entry.conflict_all < conflict_all_t::no_conflict)
					return {};

				return QBrush(conflict_all_background(entry.conflict_all));
			}

			if (role == Qt::ForegroundRole)
			{
				for (const auto & v : entry.versions)
				{
					if (v.plugin_idx != tree_[fi].plugin_idx)
						continue;

					return QBrush(conflict_this_foreground(v.status));
				}
			}

			return {};
		}
	}

	return {};
}

Qt::ItemFlags nav_tree_model_t::flags(const QModelIndex & index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	void * ptr = index.internalPointer();

	for (size_t fi = 0; fi < tree_.size(); ++fi)
	{
		for (size_t gi = 0; gi < tree_[fi].groups.size(); ++gi)
		{
			if (ptr == &tree_[fi].groups[gi])
				return f | Qt::ItemIsDragEnabled;
		}
	}

	return f;
}

Qt::DropActions nav_tree_model_t::supportedDragActions() const
{
	return Qt::CopyAction;
}

QMimeData * nav_tree_model_t::mimeData(const QModelIndexList & indexes) const
{
	if (indexes.isEmpty())
		return nullptr;

	const auto & idx = indexes.first();
	auto info = node_at(idx);

	if (info.record_id.empty())
		return nullptr;

	auto * mime = new QMimeData;
	QString payload = QString("%1\t%2\t%3")
	                      .arg(info.plugin_idx)
	                      .arg(QString::fromStdString(info.rec_type))
	                      .arg(QString::fromStdString(info.record_id));
	mime->setData("application/x-yampt-record", payload.toUtf8());
	return mime;
}

nav_tree_model_t::node_info_t nav_tree_model_t::node_at(const QModelIndex & index) const
{
	if (!index.isValid())
		return { -1, {}, {} };

	void * ptr = index.internalPointer();
	const auto & entries = scan_.entries();

	if (ptr == nullptr)
	{
		int file_idx = index.row();
		if (file_idx < 0 || file_idx >= static_cast<int>(tree_.size()))
			return { -1, {}, {} };

		return { tree_[static_cast<size_t>(file_idx)].plugin_idx, {}, {} };
	}

	for (size_t fi = 0; fi < tree_.size(); ++fi)
	{
		if (ptr == &tree_[fi])
		{
			int group_idx = index.row();
			if (group_idx < 0 || group_idx >= static_cast<int>(tree_[fi].groups.size()))
				return { tree_[fi].plugin_idx, {}, {} };

			return { tree_[fi].plugin_idx, tree_[fi].groups[static_cast<size_t>(group_idx)].type, {} };
		}

		for (size_t gi = 0; gi < tree_[fi].groups.size(); ++gi)
		{
			if (ptr != &tree_[fi].groups[gi])
				continue;

			int rec_idx = index.row();
			if (rec_idx < 0 || rec_idx >= static_cast<int>(tree_[fi].groups[gi].records.size()))
				return { tree_[fi].plugin_idx, tree_[fi].groups[gi].type, {} };

			const auto & vis = tree_[fi].groups[gi].records[static_cast<size_t>(rec_idx)];
			const auto & entry = entries[vis.entry_idx];
			return { tree_[fi].plugin_idx, entry.rec_type, entry.record_id };
		}
	}

	return { -1, {}, {} };
}
