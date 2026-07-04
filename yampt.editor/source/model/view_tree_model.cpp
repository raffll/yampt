#include "view_tree_model.hpp"
#include <decoder/view_tree_format.hpp>
#include <scanner/record_conflict.hpp>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <theme_system.hpp>
#include <QBrush>
#include <QMimeData>

view_tree_model_t::view_tree_model_t(QObject * parent)
    : QAbstractItemModel(parent)
{}

void view_tree_model_t::set_record(plugin_scan_t & scan, const conflict_entry_t & entry)
{
	beginResetModel();

	m_scan_for_header = &scan;
	m_rows.clear();
	m_column_names.clear();
	m_plugin_conflict_this.clear();
	m_record_type = entry.rec_type;
	m_record_id = entry.record_id;
	m_column_plugin_indices.clear();
	m_filter_dirty = true;
	m_has_merge_column = false;
	m_merge_col_index = -1;
	m_is_merge_pinned = scan.is_merge_pinned(entry.rec_type, entry.record_id);

	size_t col_count = setup_columns(scan, entry);
	if (col_count == 0)
	{
		endResetModel();
		return;
	}

	build_header_row(scan, entry);

	std::vector<std::vector<sub_record_view_t>> all_sub_records;
	std::vector<std::string> content_storage;
	record_context_t context { all_sub_records, content_storage, col_count };

	load_sub_records(scan, entry, context);

	const bool is_cell = (m_record_type == "CELL");
	const bool is_leveled = (m_record_type == "LEVI" || m_record_type == "LEVC");
	const bool is_faction = (m_record_type == "FACT");
	const bool is_container =
	    (m_record_type == "CONT" || m_record_type == "CREA" || m_record_type == "NPC_" || m_record_type == "BSGN" ||
	     m_record_type == "RACE");
	const bool is_armor = (m_record_type == "ARMO" || m_record_type == "CLOT");
	const bool is_dial = false;

	if (is_cell)
		set_record_cell(context);
	else if (is_leveled)
		set_record_leveled(context, entry);
	else if (is_faction)
		set_record_faction(context, entry);
	else if (is_container)
		set_record_container(context, entry);
	else if (is_armor)
		set_record_armor(context, entry);
	else if (is_dial)
		set_record_dial(scan, context, entry);
	else
		set_record_generic(context, entry);

	finalize_header_conflict();

	if (m_col_type_indices.empty())
	{
		m_col_type_indices.resize(col_count);
		for (size_t col = 0; col < col_count; ++col)
		{
			if (col >= all_sub_records.size())
				continue;

			for (size_t i = 0; i < all_sub_records[col].size(); ++i)
				m_col_type_indices[col][all_sub_records[col][i].type].push_back(i);
		}
	}

	endResetModel();
}

size_t view_tree_model_t::setup_columns(plugin_scan_t & scan, const conflict_entry_t & entry)
{
	for (const auto & ver : entry.versions)
	{
		m_column_names.push_back(scan.plugin_filename(ver.plugin_idx));
		m_plugin_conflict_this.push_back(ver.status);
		m_column_plugin_indices.push_back(ver.plugin_idx);
	}

	return entry.versions.size();
}

void view_tree_model_t::setup_merge_column(plugin_scan_t & scan, const conflict_entry_t & entry, size_t & col_count)
{
	if (!scan.has_merge())
		return;

	for (const auto & ver : entry.versions)
	{
		if (scan.is_merge_plugin(ver.plugin_idx))
			return;
	}

	m_has_merge_column = true;
	m_merge_col_index = static_cast<int>(col_count);

	int merge_idx = -1;
	for (int i = 0; i < static_cast<int>(scan.plugin_count()); ++i)
	{
		if (scan.is_merge_plugin(i))
		{
			merge_idx = i;
			break;
		}
	}

	char label_buf[64];
	std::snprintf(label_buf, sizeof(label_buf), "%s *", scan.plugin_filename(merge_idx).c_str());
	m_column_names.push_back(label_buf);
	m_plugin_conflict_this.push_back(conflict_this_t::unknown);
	m_column_plugin_indices.push_back(merge_idx);
	++col_count;
}

static std::string read_record_flags(plugin_scan_t & scan, const record_version_t & ver)
{
	static constexpr size_t record_header_size = 16;
	static constexpr size_t flags_offset = 12;

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

	if (content.size() < record_header_size)
		return "";

	uint32_t flags = 0;
	std::memcpy(&flags, content.data() + flags_offset, 4);

	std::string result;
	if (flags & 0x00000400)
	{
		if (!result.empty())
			result += " | ";

		result += "Persistent";
	}

	if (flags & 0x00002000)
	{
		if (!result.empty())
			result += " | ";

		result += "Blocked";
	}

	if (result.empty())
		return "";

	return result;
}

static bool check_all_identical(const std::vector<std::string> & values)
{
	for (size_t col = 1; col < values.size(); ++col)
	{
		if (values[col] != values[0])
			return false;
	}
	return true;
}

void view_tree_model_t::build_header_row(plugin_scan_t & scan, const conflict_entry_t & entry)
{
	const auto col_count = m_column_names.size();

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
		flags_row.values[col] = read_record_flags(scan, entry.versions[col]);

	flags_row.all_identical = check_all_identical(flags_row.values);
	flags_row.row_conflict_all = compute_conflict_all(flags_row.values);
	flags_row.cell_conflict_this = compute_conflict_this(flags_row.values);
	header_row.children.push_back(std::move(flags_row));

	m_rows.push_back(std::move(header_row));
}

void view_tree_model_t::load_sub_records(
    plugin_scan_t & scan,
    const conflict_entry_t & entry,
    record_context_t & context)
{
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
			context.all_sub_records.push_back({});
			context.content_storage.push_back({});
			continue;
		}

		context.content_storage.push_back(std::move(content));
		const auto & stored = context.content_storage.back();

		std::vector<sub_record_view_t> subs;
		sub_record_iter_t iter(stored);
		sub_record_view_t sv;
		while (iter.next(sv))
			subs.push_back(sv);

		context.all_sub_records.push_back(std::move(subs));
	}
}

void view_tree_model_t::finalize_header_conflict()
{
	if (m_rows.empty())
		return;

	auto & header = m_rows[0];
	header.row_conflict_all = conflict_all_t::only_one;
	for (const auto & child : header.children)
	{
		if (child.row_conflict_all > header.row_conflict_all)
			header.row_conflict_all = child.row_conflict_all;
	}
	header.all_identical = (header.row_conflict_all <= conflict_all_t::no_conflict);
}

void view_tree_model_t::set_excluded_plugins(const std::set<std::string> * excluded)
{
	m_excluded_plugins = excluded;
}

const std::vector<view_tree_model_t::sub_record_row_t> & view_tree_model_t::rows() const
{
	return visible_rows();
}

void view_tree_model_t::set_patch_plugins(const std::set<std::string> * patch)
{
	m_patch_plugins = patch;
}

void view_tree_model_t::clear()
{
	beginResetModel();
	m_rows.clear();
	m_column_names.clear();
	m_plugin_conflict_this.clear();
	m_column_plugin_indices.clear();
	m_record_type.clear();
	m_record_id.clear();
	m_has_merge_column = false;
	m_merge_col_index = -1;
	m_filter_dirty = true;
	endResetModel();
}

bool view_tree_model_t::is_merge_column(int section) const
{
	if (!m_has_merge_column)
		return false;

	return (section - 1) == m_merge_col_index;
}

int view_tree_model_t::merge_column() const
{
	if (!m_has_merge_column)
		return -1;

	return m_merge_col_index + 1;
}

void view_tree_model_t::set_hide_no_conflict(bool hide)
{
	beginResetModel();
	m_hide_no_conflict = hide;
	m_filter_dirty = true;
	endResetModel();
}

const std::vector<view_tree_model_t::sub_record_row_t> & view_tree_model_t::visible_rows() const
{
	if (!m_hide_no_conflict)
		return m_rows;

	if (!m_filter_dirty)
		return m_filtered_rows;

	m_filtered_rows.clear();
	for (const auto & row : m_rows)
	{
		if (!row.all_identical)
			m_filtered_rows.push_back(row);
	}

	m_filter_dirty = false;
	return m_filtered_rows;
}

QModelIndex view_tree_model_t::index(int row, int column, const QModelIndex & parent) const
{
	if (!hasIndex(row, column, parent))
		return {};

	if (!parent.isValid())
		return createIndex(row, column, nullptr);

	if (parent.internalPointer() != nullptr)
	{
		auto * sub_ptr = static_cast<sub_record_row_t *>(parent.internalPointer());
		const auto & vrows = visible_rows();
		for (size_t i = 0; i < vrows.size(); ++i)
		{
			if (&vrows[i] != sub_ptr)
				continue;

			if (parent.row() < 0 || parent.row() >= static_cast<int>(sub_ptr->children.size()))
				return {};

			return createIndex(row, column, const_cast<field_row_t *>(&sub_ptr->children[parent.row()]));
		}
		return {};
	}

	const auto & vrows = visible_rows();
	if (parent.row() < 0 || parent.row() >= static_cast<int>(vrows.size()))
		return {};

	return createIndex(row, column, const_cast<sub_record_row_t *>(&vrows[parent.row()]));
}

QModelIndex view_tree_model_t::parent(const QModelIndex & child) const
{
	if (!child.isValid())
		return {};

	void * ptr = child.internalPointer();
	if (!ptr)
		return {};

	const auto & vrows = visible_rows();

	for (int i = 0; i < static_cast<int>(vrows.size()); ++i)
	{
		if (ptr == &vrows[i])
			return createIndex(i, 0, nullptr);

		for (int j = 0; j < static_cast<int>(vrows[i].children.size()); ++j)
		{
			if (ptr == &vrows[i].children[j])
				return createIndex(j, 0, const_cast<sub_record_row_t *>(&vrows[i]));
		}
	}

	return {};
}

int view_tree_model_t::rowCount(const QModelIndex & parent) const
{
	if (parent.column() > 0)
		return 0;

	if (!parent.isValid())
		return static_cast<int>(visible_rows().size());

	void * ptr = parent.internalPointer();
	const auto & vrows = visible_rows();

	if (ptr == nullptr)
	{
		if (parent.row() < 0 || parent.row() >= static_cast<int>(vrows.size()))
			return 0;

		const auto & children = vrows[parent.row()].children;
		if (children.size() == 1 && children[0].children.empty())
			return 0;

		return static_cast<int>(children.size());
	}

	auto * sub_ptr = static_cast<sub_record_row_t *>(ptr);
	for (size_t i = 0; i < vrows.size(); ++i)
	{
		if (&vrows[i] != sub_ptr)
			continue;

		if (parent.row() < 0 || parent.row() >= static_cast<int>(sub_ptr->children.size()))
			return 0;

		const auto & field = sub_ptr->children[parent.row()];
		return static_cast<int>(field.children.size());
	}

	return 0;
}

int view_tree_model_t::columnCount(const QModelIndex &) const
{
	return static_cast<int>(m_column_names.size()) + 1;
}

static QVariant sub_record_display(const view_tree_model_t::sub_record_row_t & row, int column)
{
	if (column == 0)
		return QString::fromStdString(row.label);

	if (row.children.size() == 1)
	{
		const int col = column - 1;
		if (col < 0 || col >= static_cast<int>(row.children[0].values.size()))
			return {};

		return QString::fromStdString(row.children[0].values[col]);
	}

	if (!row.children.empty())
		return {};

	const int col = column - 1;
	if (col < 0 || col >= static_cast<int>(row.values.size()))
		return {};

	return QString::fromStdString(row.values[col]);
}

static QVariant sub_record_background(const view_tree_model_t::sub_record_row_t & row, int column)
{
	if (row.row_conflict_all < conflict_all_t::no_conflict)
		return {};

	const auto & theme = theme_system_t::instance();

	if (column > 0 && row.children.empty())
	{
		const int col = column - 1;
		if (col >= 0 && col < static_cast<int>(row.values.size()) && row.values[col].empty())
		{
			const auto active = theme.active_theme();
			const auto raw = conflict_all_color_raw(row.row_conflict_all, active);
			if (active == theme_t::dark)
				return QBrush(darker_hsl(raw, 0.78));

			return QBrush(lighter_hsl(raw, 0.93));
		}
	}

	return QBrush(theme.conflict_all_background(row.row_conflict_all));
}

static QVariant sub_record_foreground(
    const std::vector<conflict_this_t> & cell_conflicts,
    size_t column_count,
    int column,
    bool has_merge_column)
{
	const size_t real_columns = has_merge_column ? column_count - 1 : column_count;
	if (real_columns <= 1)
		return {};

	const auto & theme = theme_system_t::instance();

	if (column == 0)
	{
		conflict_this_t worst = conflict_this_t::unknown;
		for (const auto & status : cell_conflicts)
		{
			if (status == conflict_this_t::identical_to_master)
				continue;

			if (status > worst)
				worst = status;
		}

		if (worst == conflict_this_t::unknown || worst == conflict_this_t::master)
			return {};

		return QBrush(theme.conflict_this_foreground(worst));
	}

	const int col = column - 1;
	if (col < 0 || col >= static_cast<int>(cell_conflicts.size()))
		return {};

	return QBrush(theme.conflict_this_foreground(cell_conflicts[col]));
}

static QVariant field_row_display(const view_tree_model_t::field_row_t & frow, int column)
{
	if (column == 0)
		return QString::fromStdString(frow.name);

	const int col = column - 1;
	if (col < 0 || col >= static_cast<int>(frow.values.size()))
		return {};

	return QString::fromStdString(frow.values[col]);
}

static QVariant field_row_background(const view_tree_model_t::field_row_t & frow, int column)
{
	if (frow.row_conflict_all < conflict_all_t::no_conflict)
		return {};

	const auto & theme = theme_system_t::instance();

	if (column > 0)
	{
		const int col = column - 1;
		if (col >= 0 && col < static_cast<int>(frow.values.size()) && frow.values[col].empty())
		{
			const auto active = theme.active_theme();
			const auto raw = conflict_all_color_raw(frow.row_conflict_all, active);
			if (active == theme_t::dark)
				return QBrush(darker_hsl(raw, 0.78));

			return QBrush(lighter_hsl(raw, 0.93));
		}
	}

	return QBrush(theme.conflict_all_background(frow.row_conflict_all));
}

QVariant view_tree_model_t::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
		return {};

	void * ptr = index.internalPointer();

	if (!ptr)
	{
		const auto & vrows = visible_rows();
		if (index.row() < 0 || index.row() >= static_cast<int>(vrows.size()))
			return {};

		const auto & row = vrows[index.row()];

		if (role == Qt::DisplayRole)
			return sub_record_display(row, index.column());

		if (role == Qt::BackgroundRole)
			return sub_record_background(row, index.column());

		if (role == Qt::ForegroundRole)
		{
			if (m_is_merge_pinned && is_merge_column(index.column()))
				return QBrush(QColor(0, 128, 128));

			return sub_record_foreground(
			    row.cell_conflict_this, m_column_names.size(), index.column(), m_has_merge_column);
		}

		return {};
	}

	const auto & vrows = visible_rows();

	for (size_t i = 0; i < vrows.size(); ++i)
	{
		if (ptr == &vrows[i])
		{
			if (index.row() < 0 || index.row() >= static_cast<int>(vrows[i].children.size()))
				return {};

			const auto & frow = vrows[i].children[index.row()];

			if (role == Qt::DisplayRole)
				return field_row_display(frow, index.column());

			if (role == Qt::BackgroundRole)
				return field_row_background(frow, index.column());

			if (role == Qt::ForegroundRole)
			{
				if (m_is_merge_pinned && is_merge_column(index.column()))
					return QBrush(QColor(0, 128, 128));

				return sub_record_foreground(
				    frow.cell_conflict_this, m_column_names.size(), index.column(), m_has_merge_column);
			}

			return {};
		}

		for (size_t j = 0; j < vrows[i].children.size(); ++j)
		{
			if (ptr != &vrows[i].children[j])
				continue;

			if (index.row() < 0 || index.row() >= static_cast<int>(vrows[i].children[j].children.size()))
				return {};

			const auto & grand = vrows[i].children[j].children[index.row()];

			if (role == Qt::DisplayRole)
				return field_row_display(grand, index.column());

			if (role == Qt::BackgroundRole)
				return field_row_background(grand, index.column());

			if (role == Qt::ForegroundRole)
			{
				if (m_is_merge_pinned && is_merge_column(index.column()))
					return QBrush(QColor(0, 128, 128));

				return sub_record_foreground(
				    grand.cell_conflict_this, m_column_names.size(), index.column(), m_has_merge_column);
			}

			return {};
		}
	}

	return {};
}

QVariant view_tree_model_t::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal)
		return {};

	if (role == Qt::DisplayRole)
	{
		if (section == 0)
			return {};

		const int col = section - 1;
		if (col < 0 || col >= static_cast<int>(m_column_names.size()))
			return {};

		const auto & name = m_column_names[col];
		QString prefix;

		if (col < static_cast<int>(m_column_plugin_indices.size()))
		{
			const int pi = m_column_plugin_indices[col];

			if (m_excluded_plugins && m_excluded_plugins->count(name))
				prefix = QString::fromUtf8("\xF0\x9F\x94\x92 ");
			else if (m_patch_plugins && m_patch_plugins->count(name))
				prefix = QString::fromUtf8("\xF0\x9F\x9B\xA1 ");
			else if (m_scan_for_header && m_scan_for_header->is_merge_plugin(pi))
				prefix = QString::fromUtf8("\xE2\x9A\x99 ");
			else if (
			    name.size() > 4 &&
			    (name.compare(name.size() - 4, 4, ".esm") == 0 || name.compare(name.size() - 4, 4, ".ESM") == 0))
				prefix = QString::fromUtf8("\xF0\x9F\x93\x9C ");
		}

		return prefix + QString::fromStdString(name);
	}

	if (role == Qt::ForegroundRole)
	{
		if (section == 0)
			return {};

		const int col = section - 1;
		if (col < 0 || col >= static_cast<int>(m_plugin_conflict_this.size()))
			return {};

		if (m_column_names.size() <= 1)
			return {};

		if (m_is_merge_pinned && is_merge_column(section))
			return QBrush(QColor(0, 128, 128));

		const auto & theme = theme_system_t::instance();
		return QBrush(theme.conflict_this_foreground(m_plugin_conflict_this[col]));
	}

	return {};
}

Qt::ItemFlags view_tree_model_t::flags(const QModelIndex & index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	Qt::ItemFlags result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	if (index.column() > 0 && index.column() <= static_cast<int>(m_column_plugin_indices.size()))
	{
		if (!is_merge_column(index.column()))
			result |= Qt::ItemIsDragEnabled;

		if (is_merge_column(index.column()))
			result |= Qt::ItemIsDropEnabled;
	}

	return result;
}

Qt::DropActions view_tree_model_t::supportedDragActions() const
{
	return Qt::CopyAction;
}

QMimeData * view_tree_model_t::mimeData(const QModelIndexList & indexes) const
{
	Q_UNUSED(indexes);
	return nullptr;
}

Qt::DropActions view_tree_model_t::supportedDropActions() const
{
	return Qt::CopyAction;
}

bool view_tree_model_t::canDropMimeData(const QMimeData * data, Qt::DropAction, int, int, const QModelIndex &) const
{
	if (!data)
		return false;

	if (data->hasFormat("application/x-yampt-record"))
		return true;

	if (data->hasFormat("application/x-yampt-subrecord"))
		return true;

	return false;
}
