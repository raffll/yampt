#include "record_table_model.hpp"
#include <QColor>
#include <QString>
#include <algorithm>

static QString first_line(const std::string & text)
{
	auto pos = text.find_first_of("\r\n");
	if (pos == std::string::npos)
		return QString::fromStdString(text);

	return QString::fromStdString(text.substr(0, pos));
}

static std::string status_display_name(const std::string & status)
{
	if (status == "untranslated")
		return "Untranslated";
	if (status == "missing")
		return "Missing";
	if (status == "duplicate")
		return "Duplicate";
	if (status == "coords")
		return "Coords";
	if (status == "fingerprint")
		return "Fingerprint";
	if (status == "heuristic")
		return "Heuristic";
	if (status == "info")
		return "Info";
	if (status == "exact")
		return "Exact";
	if (status == "wilderness")
		return "Wilderness";
	if (status == "region")
		return "Region";
	if (status == "matched")
		return "Matched";
	if (status == "error")
		return "Error";
	if (status == "identical")
		return "Identical";
	if (status == "translated")
		return "Translated";
	if (status == "reused")
		return "Reused";
	if (status == "adapted")
		return "Adapted";
	if (status == "changed")
		return "Changed";
	if (status == "in_progress")
		return "In Progress";
	if (status == "mismatch")
		return "Mismatch";
	return status;
}

static QColor status_to_color(const std::string & status)
{
	if (status == "untranslated")
		return QColor(166, 166, 166);
	if (status == "missing")
		return QColor(242, 140, 89);
	if (status == "duplicate")
		return QColor(242, 230, 102);
	if (status == "coords")
		return QColor(102, 204, 204);
	if (status == "fingerprint")
		return QColor(102, 191, 217);
	if (status == "heuristic")
		return QColor(153, 179, 230);
	if (status == "info")
		return QColor(115, 217, 191);
	if (status == "exact")
		return QColor(128, 230, 217);
	if (status == "wilderness")
		return QColor(77, 153, 77);
	if (status == "region")
		return QColor(153, 153, 89);
	if (status == "matched")
		return QColor(217, 217, 217);
	if (status == "error")
		return QColor(242, 102, 102);
	if (status == "identical")
		return QColor(140, 191, 140);
	if (status == "translated")
		return QColor(128, 230, 128);
	if (status == "reused")
		return QColor(128, 217, 179);
	if (status == "adapted")
		return QColor(179, 140, 217);
	if (status == "changed")
		return QColor(242, 179, 102);
	if (status == "in_progress")
		return QColor(102, 153, 242);
	return QColor(217, 217, 217);
}

int record_table_model_t::rowCount(const QModelIndex & parent) const
{
	if (parent.isValid())
		return 0;

	return static_cast<int>(rows_.size());
}

int record_table_model_t::columnCount(const QModelIndex & parent) const
{
	if (parent.isValid())
		return 0;

	return 5;
}

QVariant record_table_model_t::data(const QModelIndex & index, int role) const
{
	if (!index.isValid())
		return {};

	if (index.row() < 0 || index.row() >= static_cast<int>(rows_.size()))
		return {};

	const auto & row = rows_[index.row()];

	if (role == Qt::DisplayRole)
	{
		switch (index.column())
		{
		case 0:
			return QString::fromStdString(tools_t::type_to_str(row.type));
		case 1:
			return QString::fromStdString(row.key_text);
		case 2:
			return first_line(row.old_text);
		case 3:
			return first_line(row.new_text);
		case 4:
			return QString::fromStdString(status_display_name(row.status));
		default:
			return {};
		}
	}

	if (role == Qt::BackgroundRole && index.column() == 4)
		return status_to_color(row.status);

	return {};
}

QVariant record_table_model_t::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return {};

	switch (section)
	{
	case 0:
		return QStringLiteral("Type");
	case 1:
		return QStringLiteral("Key");
	case 2:
		return QStringLiteral("Original");
	case 3:
		return QStringLiteral("Translation");
	case 4:
		return QStringLiteral("Status");
	default:
		return {};
	}
}

void record_table_model_t::sort(int column, Qt::SortOrder order)
{
	beginResetModel();

	auto cmp = [column, order](const table_row_t & a, const table_row_t & b)
	{
		int result = 0;

		switch (column)
		{
		case 0:
			result = static_cast<int>(a.type) - static_cast<int>(b.type);
			break;
		case 1:
			result = a.key_text.compare(b.key_text);
			break;
		case 2:
			result = a.old_text.compare(b.old_text);
			break;
		case 3:
			result = a.new_text.compare(b.new_text);
			break;
		case 4:
			result = a.status.compare(b.status);
			break;
		}

		if (order == Qt::DescendingOrder)
			return result > 0;

		return result < 0;
	};

	std::sort(rows_.begin(), rows_.end(), cmp);
	endResetModel();
}

void record_table_model_t::rebuild(std::vector<table_row_t> rows)
{
	beginResetModel();
	rows_ = std::move(rows);
	endResetModel();
}

const table_row_t * record_table_model_t::row_at(int row) const
{
	if (row < 0 || row >= static_cast<int>(rows_.size()))
		return nullptr;

	return &rows_[row];
}

void record_table_model_t::update_row(int row, const std::string & new_text, const std::string & status)
{
	if (row < 0 || row >= static_cast<int>(rows_.size()))
		return;

	rows_[row].new_text = new_text;
	rows_[row].status = status;
	emit dataChanged(index(row, 3), index(row, 4));
}
