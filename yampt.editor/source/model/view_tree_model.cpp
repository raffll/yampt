#include "view_tree_model.hpp"
#include <plugin_scan/view_tree_format.hpp>
#include <plugin_scan/conflict_compute.hpp>
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

	size_t col_count = setup_columns(scan, entry);
	setup_merge_column(scan, entry, col_count);
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

	const bool is_cell = (record_type_ == "CELL");
	const bool is_leveled = (record_type_ == "LEVI" || record_type_ == "LEVC");
	const bool is_faction = (record_type_ == "FACT");
	const bool is_container =
	    (record_type_ == "CONT" || record_type_ == "CREA" || record_type_ == "NPC_" || record_type_ == "BSGN");

	if (is_cell)
		set_record_cell(context);
	else if (is_leveled)
		set_record_leveled(context, entry);
	else if (is_faction)
		set_record_faction(context, entry);
	else if (is_container)
		set_record_container(context, entry);
	else
		set_record_generic(context, entry);

	finalize_header_conflict();
	endResetModel();
}

size_t view_tree_model_t::setup_columns(plugin_scan_t & scan, const conflict_entry_t & entry)
{
	for (const auto & ver : entry.versions)
	{
		char label_buf[64];
		std::snprintf(
		    label_buf, sizeof(label_buf), "[%02X] %s", ver.plugin_idx, scan.plugin_filename(ver.plugin_idx).c_str());
		column_names_.push_back(label_buf);
		plugin_conflict_this_.push_back(ver.status);
		column_plugin_indices_.push_back(ver.plugin_idx);
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

	char label_buf[64];
	std::snprintf(label_buf, sizeof(label_buf), "[%02X] %s *", merge_idx, scan.plugin_filename(merge_idx).c_str());
	column_names_.push_back(label_buf);
	plugin_conflict_this_.push_back(conflict_this_t::unknown);
	column_plugin_indices_.push_back(merge_idx);
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
	char flags_buf[16];
	std::snprintf(flags_buf, sizeof(flags_buf), "0x%08X", flags);
	return flags_buf;
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
	const auto col_count = column_names_.size();

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

	rows_.push_back(std::move(header_row));
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
	if (rows_.empty())
		return;

	conflict_all_t worst = conflict_all_t::only_one;
	for (size_t i = 1; i < rows_.size(); ++i)
	{
		if (rows_[i].row_conflict_all > worst)
			worst = rows_[i].row_conflict_all;
	}
	rows_[0].row_conflict_all = worst;
	rows_[0].all_identical = (worst <= conflict_all_t::no_conflict);
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

static QVariant sub_record_display(const view_tree_model_t::sub_record_row_t & row, int column)
{
	if (column == 0)
		return QString::fromStdString(row.label);

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

	if (column > 0)
	{
		const int col = column - 1;
		if (col >= 0 && col < static_cast<int>(row.values.size()) && row.values[col].empty())
			return QBrush(lighter_hsl(conflict_all_color_raw(row.row_conflict_all), 0.93));
	}

	return QBrush(conflict_all_background(row.row_conflict_all));
}

static QVariant sub_record_foreground(
    const std::vector<conflict_this_t> & cell_conflicts,
    size_t column_count,
    int column)
{
	if (column_count <= 1)
		return {};

	if (column == 0)
	{
		conflict_this_t worst = conflict_this_t::unknown;
		for (const auto & status : cell_conflicts)
		{
			if (status > worst)
				worst = status;
		}
		return QBrush(conflict_this_foreground(worst));
	}

	const int col = column - 1;
	if (col < 0 || col >= static_cast<int>(cell_conflicts.size()))
		return {};

	return QBrush(conflict_this_foreground(cell_conflicts[col]));
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

	if (column > 0)
	{
		const int col = column - 1;
		if (col >= 0 && col < static_cast<int>(frow.values.size()) && frow.values[col].empty())
			return QBrush(lighter_hsl(conflict_all_color_raw(frow.row_conflict_all), 0.93));
	}

	return QBrush(conflict_all_background(frow.row_conflict_all));
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
			return sub_record_display(row, index.column());

		if (role == Qt::BackgroundRole)
			return sub_record_background(row, index.column());

		if (role == Qt::ForegroundRole)
			return sub_record_foreground(row.cell_conflict_this, column_names_.size(), index.column());

		return {};
	}

	if (index.row() < 0 || index.row() >= static_cast<int>(parent_ptr->children.size()))
		return {};

	const auto & frow = parent_ptr->children[index.row()];

	if (role == Qt::DisplayRole)
		return field_row_display(frow, index.column());

	if (role == Qt::BackgroundRole)
		return field_row_background(frow, index.column());

	if (role == Qt::ForegroundRole)
		return sub_record_foreground(frow.cell_conflict_this, column_names_.size(), index.column());

	return {};
}

QVariant view_tree_model_t::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return {};

	if (section == 0)
		return QStringLiteral("Sub-Record");

	const int col = section - 1;
	if (col < 0 || col >= static_cast<int>(column_names_.size()))
		return {};

	return QString::fromStdString(column_names_[col]);
}

Qt::ItemFlags view_tree_model_t::flags(const QModelIndex & index) const
{
	if (!index.isValid())
		return Qt::NoItemFlags;

	Qt::ItemFlags result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

	if (index.column() > 0 && index.column() <= static_cast<int>(column_plugin_indices_.size()))
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
	if (indexes.isEmpty())
		return nullptr;

	const auto & first_idx = indexes.first();
	if (first_idx.column() <= 0)
		return nullptr;

	const int col = first_idx.column() - 1;
	if (col < 0 || col >= static_cast<int>(column_plugin_indices_.size()))
		return nullptr;

	if (is_merge_column(first_idx.column()))
		return nullptr;

	const int plugin_idx = column_plugin_indices_[col];
	auto * mime_data = new QMimeData;
	QString payload = QString("%1\t%2\t%3")
	                      .arg(plugin_idx)
	                      .arg(QString::fromStdString(record_type_))
	                      .arg(QString::fromStdString(record_id_));
	mime_data->setData("application/x-yampt-record", payload.toUtf8());
	return mime_data;
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
