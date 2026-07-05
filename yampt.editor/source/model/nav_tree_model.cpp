#include "nav_tree_model.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <set>
#include <theme_system.hpp>
#include <QFont>

static int conflict_this_priority(conflict_this_t ct)
{
	switch (ct)
	{
	case conflict_this_t::unknown:
		return 0;
	case conflict_this_t::identical_to_master:
		return 1;
	case conflict_this_t::master:
		return 2;
	case conflict_this_t::override_wins:
		return 3;
	case conflict_this_t::conflict_wins:
		return 4;
	case conflict_this_t::conflict_loses:
		return 5;
	case conflict_this_t::deleted:
		return 0;
	default:
		return 0;
	}
}

static size_t unique_plugin_count(const conflict_entry_t & entry)
{
	std::set<int> plugins;
	for (const auto & v : entry.versions)
		plugins.insert(v.plugin_idx);
	return plugins.size();
}

conflict_this_t nav_tree_model_t::record_foreground_for_plugin(const conflict_entry_t & entry, int plugin_idx) const
{
	if (entry.versions.size() <= 1)
		return conflict_this_t::unknown;

	if (m_filter.hide_duplicates() && unique_plugin_count(entry) <= 1)
		return conflict_this_t::unknown;

	for (const auto & v : entry.versions)
	{
		if (v.plugin_idx != plugin_idx)
			continue;

		if (v.status == conflict_this_t::deleted)
			return conflict_this_t::deleted;

		return v.status;
	}

	return conflict_this_t::unknown;
}

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
    , m_scan(scan)
{}

void nav_tree_model_t::rebuild()
{
	beginResetModel();
	build_tree();
	endResetModel();
}

void nav_tree_model_t::set_filter(const filter_state_t & state)
{
	m_filter.set_filter(state);
	rebuild();
}

void nav_tree_model_t::clear_filter()
{
	m_filter.clear();
	rebuild();
}

void nav_tree_model_t::set_hide_duplicates(bool hide)
{
	m_filter.set_hide_duplicates(hide);
	emit dataChanged(index(0, 0, {}), index(rowCount({}) - 1, columnCount({}) - 1, {}));
}

void nav_tree_model_t::set_excluded_plugins(const std::set<std::string> * excluded)
{
	m_filter.set_excluded_plugins(excluded);
}

void nav_tree_model_t::set_patch_plugins(const std::set<std::string> * patch)
{
	m_filter.set_patch_plugins(patch);
}

void nav_tree_model_t::build_tree()
{
	m_tree.clear();

	const auto & entries = m_scan.entries();

	for (int p = 0; p < static_cast<int>(m_scan.plugin_count()); ++p)
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

			if (!m_filter.passes(entry, p))
				continue;

			type_map[entry.rec_type].push_back({ ei });
		}

		for (auto & [type, recs] : type_map)
		{
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

		m_tree.push_back(std::move(file_node));
	}

	sort_records();
}

QModelIndex nav_tree_model_t::index(int row, int column, const QModelIndex & parent) const
{
	if (column < 0 || column >= 3)
		return {};

	if (!parent.isValid())
	{
		if (row < 0 || row >= static_cast<int>(m_tree.size()))
			return {};

		return createIndex(row, column, nullptr);
	}

	void * ptr = parent.internalPointer();

	if (ptr == nullptr)
	{
		int file_idx = parent.row();
		if (file_idx < 0 || file_idx >= static_cast<int>(m_tree.size()))
			return {};

		const auto & file_node = m_tree[static_cast<size_t>(file_idx)];
		if (row < 0 || row >= static_cast<int>(file_node.groups.size()))
			return {};

		return createIndex(row, column, const_cast<file_node_t *>(&file_node));
	}

	const auto * file_ptr = static_cast<const file_node_t *>(ptr);
	for (size_t fi = 0; fi < m_tree.size(); ++fi)
	{
		if (&m_tree[fi] == file_ptr)
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

	for (size_t fi = 0; fi < m_tree.size(); ++fi)
	{
		if (ptr == &m_tree[fi])
			return createIndex(static_cast<int>(fi), 0, nullptr);

		for (size_t gi = 0; gi < m_tree[fi].groups.size(); ++gi)
		{
			if (ptr == &m_tree[fi].groups[gi])
				return createIndex(static_cast<int>(gi), 0, const_cast<file_node_t *>(&m_tree[fi]));
		}
	}

	return {};
}

int nav_tree_model_t::rowCount(const QModelIndex & parent) const
{
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		return static_cast<int>(m_tree.size());

	void * ptr = parent.internalPointer();

	if (ptr == nullptr)
	{
		int file_idx = parent.row();
		if (file_idx < 0 || file_idx >= static_cast<int>(m_tree.size()))
			return 0;

		return static_cast<int>(m_tree[static_cast<size_t>(file_idx)].groups.size());
	}

	for (size_t fi = 0; fi < m_tree.size(); ++fi)
	{
		if (ptr == &m_tree[fi])
		{
			int group_idx = parent.row();
			if (group_idx < 0 || group_idx >= static_cast<int>(m_tree[fi].groups.size()))
				return 0;

			return static_cast<int>(m_tree[fi].groups[static_cast<size_t>(group_idx)].records.size());
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
		return QStringLiteral("ID");
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
	const auto & entries = m_scan.entries();
	int col = index.column();

	if (ptr == nullptr)
	{
		int file_idx = index.row();
		if (file_idx < 0 || file_idx >= static_cast<int>(m_tree.size()))
			return {};

		const auto & file_node = m_tree[static_cast<size_t>(file_idx)];

		if (role == Qt::DisplayRole && col == 0)
		{
			char buf[64];
			std::snprintf(
			    buf,
			    sizeof(buf),
			    "[%03d] %s",
			    file_node.plugin_idx,
			    m_scan.plugin_filename(file_node.plugin_idx).c_str());

			const auto & filename = m_scan.plugin_filename(file_node.plugin_idx);
			if (m_filter.excluded_plugins() && m_filter.excluded_plugins()->count(filename))
				return QString::fromUtf8("\xF0\x9F\x94\x92 ") + QString::fromUtf8(buf);

			if (m_filter.patch_plugins() && m_filter.patch_plugins()->count(filename))
				return QString::fromUtf8("\xF0\x9F\x9B\xA1 ") + QString::fromUtf8(buf);

			if (m_scan.is_merge_plugin(file_node.plugin_idx))
				return QString::fromUtf8("\xE2\x9A\x99 ") + QString::fromUtf8(buf);

			const bool is_master = filename.size() > 4 && (filename.compare(filename.size() - 4, 4, ".esm") == 0 ||
			                                               filename.compare(filename.size() - 4, 4, ".ESM") == 0);
			if (is_master)
				return QString::fromUtf8("\xF0\x9F\x93\x9C ") + QString::fromUtf8(buf);

			return QString::fromUtf8("\xF0\x9F\x93\x84 ") + QString::fromUtf8(buf);
		}

		if (role == Qt::BackgroundRole || role == Qt::ForegroundRole || role == Qt::FontRole)
		{
			conflict_all_t worst_all = conflict_all_t::only_one;
			conflict_this_t worst_this = conflict_this_t::unknown;

			for (const auto & group : file_node.groups)
			{
				for (const auto & rec : group.records)
				{
					const auto & e = entries[rec.entry_idx];
					const auto ct = record_foreground_for_plugin(e, file_node.plugin_idx);

					if (conflict_this_priority(ct) > conflict_this_priority(worst_this))
						worst_this = ct;

					if (e.conflict_all > worst_all)
						worst_all = e.conflict_all;
				}
			}

			if (role == Qt::BackgroundRole)
			{
				if (worst_all < conflict_all_t::no_conflict)
					return {};

				return QBrush(theme_system_t::instance().conflict_all_background(worst_all));
			}

			if (role == Qt::ForegroundRole)
			{
				if (worst_this == conflict_this_t::unknown)
					return {};

				return QBrush(theme_system_t::instance().conflict_this_foreground(worst_this));
			}

			if (role == Qt::FontRole && worst_this == conflict_this_t::deleted)
			{
				QFont font;
				font.setStrikeOut(true);
				return font;
			}
		}

		if (role == Qt::ToolTipRole)
		{
			size_t itm = m_scan.itm_count(file_node.plugin_idx);
			if (itm > 0)
				return QString("%1 ITM records").arg(itm);
		}

		return {};
	}

	for (size_t fi = 0; fi < m_tree.size(); ++fi)
	{
		if (ptr == &m_tree[fi])
		{
			int group_idx = index.row();
			if (group_idx < 0 || group_idx >= static_cast<int>(m_tree[fi].groups.size()))
				return {};

			const auto & group = m_tree[fi].groups[static_cast<size_t>(group_idx)];

			if (role == Qt::DisplayRole && col == 0)
			{
				const char * display_name = type_to_display_name(group.type);
				if (display_name)
					return QString("%1 [%2]").arg(display_name).arg(group.records.size());

				return QString("%1 (%2)").arg(QString::fromStdString(group.type)).arg(group.records.size());
			}

			if (role == Qt::BackgroundRole || role == Qt::ForegroundRole || role == Qt::FontRole)
			{
				conflict_all_t worst_all = conflict_all_t::only_one;
				conflict_this_t worst_this = conflict_this_t::unknown;

				for (const auto & rec : group.records)
				{
					const auto & e = entries[rec.entry_idx];
					const auto ct = record_foreground_for_plugin(e, m_tree[fi].plugin_idx);

					if (conflict_this_priority(ct) > conflict_this_priority(worst_this))
						worst_this = ct;

					if (e.conflict_all > worst_all)
						worst_all = e.conflict_all;
				}

				if (role == Qt::BackgroundRole)
				{
					if (worst_all < conflict_all_t::no_conflict)
						return {};

					return QBrush(theme_system_t::instance().conflict_all_background(worst_all));
				}

				if (role == Qt::ForegroundRole)
				{
					if (worst_this == conflict_this_t::unknown)
						return {};

					return QBrush(theme_system_t::instance().conflict_this_foreground(worst_this));
				}

				if (role == Qt::FontRole && worst_this == conflict_this_t::deleted)
				{
					QFont font;
					font.setStrikeOut(true);
					return font;
				}
			}

			return {};
		}

		for (size_t gi = 0; gi < m_tree[fi].groups.size(); ++gi)
		{
			if (ptr != &m_tree[fi].groups[gi])
				continue;

			int rec_idx = index.row();
			if (rec_idx < 0 || rec_idx >= static_cast<int>(m_tree[fi].groups[gi].records.size()))
				return {};

			const auto & vis = m_tree[fi].groups[gi].records[static_cast<size_t>(rec_idx)];
			const auto & entry = entries[vis.entry_idx];
			const auto record_color = record_foreground_for_plugin(entry, m_tree[fi].plugin_idx);

			if (role == Qt::DisplayRole)
			{
				if (col == 0)
					return QString::fromStdString(entry.record_id);

				if (col == 1)
				{
					if (!entry.display_name.empty())
						return QString::fromStdString(entry.display_name);

					return QString::fromStdString(entry.dial_name);
				}
			}

			if (role == Qt::BackgroundRole)
			{
				if (entry.conflict_all < conflict_all_t::no_conflict)
					return {};

				if (m_filter.hide_duplicates() && unique_plugin_count(entry) <= 1)
					return {};

				return QBrush(theme_system_t::instance().conflict_all_background(entry.conflict_all));
			}

			if (role == Qt::ForegroundRole)
			{
				if (record_color == conflict_this_t::unknown)
					return {};

				return QBrush(theme_system_t::instance().conflict_this_foreground(record_color));
			}

			if (role == Qt::FontRole && record_color == conflict_this_t::deleted)
			{
				QFont font;
				font.setStrikeOut(true);
				return font;
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

	for (size_t fi = 0; fi < m_tree.size(); ++fi)
	{
		for (size_t gi = 0; gi < m_tree[fi].groups.size(); ++gi)
		{
			if (ptr == &m_tree[fi].groups[gi])
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

QModelIndex nav_tree_model_t::find_index(const std::string & rec_type, const std::string & record_id) const
{
	if (rec_type.empty() || record_id.empty())
		return {};

	const auto & entries = m_scan.entries();

	for (size_t fi = 0; fi < m_tree.size(); ++fi)
	{
		for (size_t gi = 0; gi < m_tree[fi].groups.size(); ++gi)
		{
			const auto & group = m_tree[fi].groups[gi];
			if (group.type != rec_type)
				continue;

			for (size_t ri = 0; ri < group.records.size(); ++ri)
			{
				const auto & entry = entries[group.records[ri].entry_idx];
				if (entry.record_id != record_id)
					continue;

				return createIndex(static_cast<int>(ri), 0, const_cast<type_group_t *>(&group));
			}
		}
	}

	return {};
}

nav_tree_model_t::node_info_t nav_tree_model_t::node_at(const QModelIndex & index) const
{
	if (!index.isValid())
		return { -1, {}, {} };

	void * ptr = index.internalPointer();
	const auto & entries = m_scan.entries();

	if (ptr == nullptr)
	{
		int file_idx = index.row();
		if (file_idx < 0 || file_idx >= static_cast<int>(m_tree.size()))
			return { -1, {}, {} };

		return { m_tree[static_cast<size_t>(file_idx)].plugin_idx, {}, {} };
	}

	for (size_t fi = 0; fi < m_tree.size(); ++fi)
	{
		if (ptr == &m_tree[fi])
		{
			int group_idx = index.row();
			if (group_idx < 0 || group_idx >= static_cast<int>(m_tree[fi].groups.size()))
				return { m_tree[fi].plugin_idx, {}, {} };

			return { m_tree[fi].plugin_idx, m_tree[fi].groups[static_cast<size_t>(group_idx)].type, {} };
		}

		for (size_t gi = 0; gi < m_tree[fi].groups.size(); ++gi)
		{
			if (ptr != &m_tree[fi].groups[gi])
				continue;

			int rec_idx = index.row();
			if (rec_idx < 0 || rec_idx >= static_cast<int>(m_tree[fi].groups[gi].records.size()))
				return { m_tree[fi].plugin_idx, m_tree[fi].groups[gi].type, {} };

			const auto & vis = m_tree[fi].groups[gi].records[static_cast<size_t>(rec_idx)];
			const auto & entry = entries[vis.entry_idx];
			return { m_tree[fi].plugin_idx, entry.rec_type, entry.record_id };
		}
	}

	return { -1, {}, {} };
}

void nav_tree_model_t::sort(int column, Qt::SortOrder order)
{
	if (column < 0 || column > 1)
		return;

	m_sort_column = column;
	m_sort_order = order;

	emit layoutAboutToBeChanged();
	sort_records();
	emit layoutChanged();
}

void nav_tree_model_t::sort_records()
{
	const auto & entries = m_scan.entries();
	const int col = m_sort_column;
	const bool ascending = (m_sort_order == Qt::AscendingOrder);

	for (auto & file_node : m_tree)
	{
		for (auto & group : file_node.groups)
		{
			std::sort(
			    group.records.begin(),
			    group.records.end(),
			    [&](const visible_record_t & a, const visible_record_t & b)
			{
				const auto & ea = entries[a.entry_idx];
				const auto & eb = entries[b.entry_idx];

				const std::string & sa = (col == 0) ? ea.record_id : ea.display_name;
				const std::string & sb = (col == 0) ? eb.record_id : eb.display_name;

				const int cmp = natural_compare(sa, sb);
				return ascending ? (cmp < 0) : (cmp > 0);
			});
		}
	}
}
