#include "record_table_model.hpp"
#include "../view/status_colors.hpp"
#include "../view/status_display.hpp"
#include <QString>
#include <algorithm>

static QString first_line(const std::string & text)
{
	auto pos = text.find_first_of("\r\n");
	if (pos == std::string::npos)
		return QString::fromStdString(text);

	return QString::fromStdString(text.substr(0, pos));
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

static QString strip_leading_whitespace(QString text)
{
	int index = 0;
	while (index < text.length() && (text[index] == ' ' || text[index] == '\t'))
		++index;

	return text.mid(index);
}

static bool is_script_type(tools_t::rec_type_t type)
{
	return type == tools_t::rec_type_t::sctx || type == tools_t::rec_type_t::bnam;
}

static QString format_key_display(const table_row_t & row)
{
	auto display = QString::fromStdString(row.key_text);
	if (is_script_type(row.type))
	{
		auto parts = display.split('^');
		for (auto & part : parts)
			part = strip_leading_whitespace(part);

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

static QString format_text_column(const table_row_t & row, const std::string & source_text)
{
	auto text = first_line(source_text);
	if (is_script_type(row.type))
		text = strip_leading_whitespace(text);

	return text;
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
			return format_key_display(row);
		case col_original:
			return format_text_column(row, row.old_text);
		case col_translation:
			if (row.status == status_t::untranslated)
				return {};

			return format_text_column(row, row.new_text);
		case col_status:
			return status_display_name(row.status);
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
			result = static_cast<int>(a.status) - static_cast<int>(b.status);
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

	std::sort(
	    groups.begin(),
	    groups.end(),
	    [&cmp](const std::vector<table_row_t> & a, const std::vector<table_row_t> & b)
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

int record_table_model_t::row_count() const
{
	return static_cast<int>(rows_.size());
}

const table_row_t * record_table_model_t::row_at(int row) const
{
	if (row < 0 || row >= static_cast<int>(rows_.size()))
		return nullptr;

	return &rows_[row];
}

void record_table_model_t::update_row(int row, const std::string & new_text, status_t status)
{
	if (row < 0 || row >= static_cast<int>(rows_.size()))
		return;

	rows_[row].new_text = new_text;
	rows_[row].status = status;
	emit dataChanged(index(row, col_id), index(row, col_status));
}
