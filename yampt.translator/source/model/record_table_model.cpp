#include "record_table_model.hpp"
#include "../view/status_display.hpp"
#include <algorithm>
#include <theme_system.hpp>
#include <QString>

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

	return static_cast<int>(m_rows.size());
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

static bool is_script_type(rec_type_t type)
{
	return type == rec_type_t::sctx || type == rec_type_t::bnam;
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

	if (index.row() < 0 || index.row() >= static_cast<int>(m_rows.size()))
		return {};

	const auto & row = m_rows[index.row()];

	if (role == Qt::DisplayRole || role == Qt::EditRole)
	{
		switch (index.column())
		{
		case col_id:
			return QString::fromStdString(domain_types::type_to_str(row.type));
		case col_key:
			return format_key_display(row);
		case col_original:
			return format_text_column(row, row.old_text);
		case col_translation:
			if (role == Qt::EditRole)
			{
				if (row.status == status_t::untranslated)
					return QString();

				return QString::fromStdString(row.new_text);
			}

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
		const auto & color = theme_system_t::instance().get_status_color(row.status);
		const auto & theme = theme_system_t::instance();

		if (theme.active_theme() == theme_t::dark)
		{
			return QColor(color.red() * 30 / 100, color.green() * 30 / 100, color.blue() * 30 / 100);
		}

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
	for (auto & row : m_rows)
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

	m_rows.clear();
	for (auto & group : groups)
	{
		for (auto & row : group)
			m_rows.push_back(std::move(row));
	}

	endResetModel();
}

void record_table_model_t::rebuild(std::vector<table_row_t> rows)
{
	beginResetModel();
	m_rows = std::move(rows);
	endResetModel();
}

int record_table_model_t::row_count() const
{
	return static_cast<int>(m_rows.size());
}

const table_row_t * record_table_model_t::row_at(int row) const
{
	if (row < 0 || row >= static_cast<int>(m_rows.size()))
		return nullptr;

	return &m_rows[row];
}

void record_table_model_t::update_row(int row, const std::string & new_text, status_t status)
{
	if (row < 0 || row >= static_cast<int>(m_rows.size()))
		return;

	m_rows[row].new_text = new_text;
	m_rows[row].status = status;
	emit dataChanged(index(row, col_id), index(row, col_status));
}

Qt::ItemFlags record_table_model_t::flags(const QModelIndex & index) const
{
	auto base_flags = QAbstractTableModel::flags(index);

	if (!index.isValid())
		return base_flags;

	if (index.column() != col_translation)
		return base_flags;

	const auto row_idx = index.row();
	if (row_idx < 0 || row_idx >= static_cast<int>(m_rows.size()))
		return base_flags;

	const auto & row = m_rows[row_idx];
	if (is_script_type(row.type))
		return base_flags;

	if (row.old_text.find_first_of("\r\n") != std::string::npos)
		return base_flags;

	return base_flags | Qt::ItemIsEditable;
}

bool record_table_model_t::setData(const QModelIndex & index, const QVariant & value, int role)
{
	if (!index.isValid() || role != Qt::EditRole)
		return false;

	if (index.column() != col_translation)
		return false;

	const auto row_idx = index.row();
	if (row_idx < 0 || row_idx >= static_cast<int>(m_rows.size()))
		return false;

	const auto new_text = value.toString().toStdString();
	if (new_text == m_rows[row_idx].new_text)
		return false;

	emit inline_edit_committed(row_idx, new_text);
	return true;
}
