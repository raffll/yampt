#include "record_table_model.hpp"
#include "status_colors.hpp"
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
	if (status == "matched" || status == "fingerprint" || status == "coords" ||
		status == "heuristic" || status == "exact" || status == "info" ||
		status == "wilderness" || status == "region")
		return "Matched";
	if (status == "error")
		return "Error";
	if (status == "identical" || status == "translated")
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
	if (status == "propagated")
		return "Propagated";
	return status;
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

	return col_count;
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
		case col_id:
			return QString::fromStdString(tools_t::type_to_str(row.type));
		case col_key:
		{
			auto display = QString::fromStdString(row.key_text);
			if (row.type == tools_t::rec_type_t::sctx || row.type == tools_t::rec_type_t::bnam)
			{
				auto parts = display.split('^');
				for (auto & part : parts)
				{
					int i = 0;
					while (i < part.length() && (part[i] == ' ' || part[i] == '\t'))
						++i;
					part = part.mid(i);
				}
				display = parts.join(QString::fromUtf8(" \xe2\x80\xa2 "));
			}
			else
			{
				display.replace('^', QString::fromUtf8(" \xe2\x80\xa2 "));
			}

			if (row.is_child)
				return QString::fromUtf8("\xe2\x86\xb3 ") + display;

			return display;
		}
		case col_original:
		{
			auto text = first_line(row.old_text);
			if (row.type == tools_t::rec_type_t::sctx || row.type == tools_t::rec_type_t::bnam)
			{
				int i = 0;
				while (i < text.length() && (text[i] == ' ' || text[i] == '\t'))
					++i;
				text = text.mid(i);
			}
			return text;
		}
		case col_translation:
		{
			auto text = first_line(row.new_text);
			if (row.type == tools_t::rec_type_t::sctx || row.type == tools_t::rec_type_t::bnam)
			{
				int i = 0;
				while (i < text.length() && (text[i] == ' ' || text[i] == '\t'))
					++i;
				text = text.mid(i);
			}
			return text;
		}
		case col_status:
			return QString::fromStdString(status_display_name(row.status));
		default:
			return {};
		}
	}

	if (role == Qt::BackgroundRole)
	{
		const auto & color = get_status_color(row.status);
		return QColor(
			255 - (255 - color.red()) * 20 / 100,
			255 - (255 - color.green()) * 20 / 100,
			255 - (255 - color.blue()) * 20 / 100);
	}

	return {};
}

QVariant record_table_model_t::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
		return {};

	switch (section)
	{
	case col_id:
		return QStringLiteral("ID");
	case col_key:
		return QStringLiteral("Key");
	case col_original:
		return QStringLiteral("Original");
	case col_translation:
		return QStringLiteral("Translation");
	case col_status:
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
		case col_id:
			result = static_cast<int>(a.type) - static_cast<int>(b.type);
			break;
		case col_key:
			result = a.key_text.compare(b.key_text);
			break;
		case col_original:
			result = a.old_text.compare(b.old_text);
			break;
		case col_translation:
			result = a.new_text.compare(b.new_text);
			break;
		case col_status:
			result = a.status.compare(b.status);
			break;
		}

		if (order == Qt::DescendingOrder)
			return result > 0;

		return result < 0;
	};

	std::vector<std::vector<table_row_t>> groups;
	for (auto & row : rows_)
	{
		if (!row.is_child)
			groups.push_back({});

		if (!groups.empty())
			groups.back().push_back(std::move(row));
	}

	std::sort(groups.begin(), groups.end(), [&cmp](const std::vector<table_row_t> & a, const std::vector<table_row_t> & b)
	{
		if (a.empty() || b.empty())
			return !a.empty();

		return cmp(a.front(), b.front());
	});

	rows_.clear();
	for (auto & group : groups)
	{
		for (auto & row : group)
			rows_.push_back(std::move(row));
	}

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
	emit dataChanged(index(row, col_id), index(row, col_status));
}
