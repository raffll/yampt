#include "sub_record_rules_view.hpp"
#include <settings_store.hpp>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPainter>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

static constexpr int col_record = 0;
static constexpr int col_sub_record = 1;
static constexpr int col_ignore_conflict = 2;
static constexpr int col_skip_missing = 3;
static constexpr int col_exclude_merge = 4;
static constexpr int col_spacer = 5;
static constexpr int column_count = 6;
static constexpr int row_height = 26;
static constexpr int header_height = 120;

class vertical_header_t : public QHeaderView
{
public:
	explicit vertical_header_t(QWidget * parent = nullptr)
	    : QHeaderView(Qt::Horizontal, parent)
	{}

	QSize sectionSizeFromContents(int logical_index) const override
	{
		if (logical_index == col_spacer)
			return { 0, header_height };

		auto size = QHeaderView::sectionSizeFromContents(logical_index);
		size.setHeight(header_height);
		return size;
	}

protected:
	void paintSection(QPainter * painter, const QRect & rect, int logical_index) const override
	{
		if (logical_index == col_spacer)
		{
			auto style_option = QStyleOptionHeader();
			initStyleOption(&style_option);
			style_option.rect = rect;
			style_option.section = logical_index;
			style_option.text = "";
			style()->drawControl(QStyle::CE_Header, &style_option, painter, this);
			return;
		}

		painter->save();

		auto style_option = QStyleOptionHeader();
		initStyleOption(&style_option);
		style_option.rect = rect;
		style_option.section = logical_index;
		style_option.text = "";
		style()->drawControl(QStyle::CE_Header, &style_option, painter, this);

		const auto text = model()->headerData(logical_index, Qt::Horizontal).toString();
		const auto font_metrics = QFontMetrics(painter->font());
		const auto text_width = font_metrics.horizontalAdvance(text);

		painter->translate(
		    rect.x() + (rect.width() + font_metrics.height()) / 2 - 2,
		    rect.y() + rect.height() - (rect.height() - text_width) / 2);
		painter->rotate(-90);
		painter->drawText(0, 0, text);

		painter->restore();
	}
};

static QWidget * make_centered_checkbox(QWidget * parent, bool checked)
{
	auto * container = new QWidget(parent);
	auto * layout = new QHBoxLayout(container);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setAlignment(Qt::AlignCenter);
	auto * checkbox = new QCheckBox(container);
	checkbox->setChecked(checked);
	layout->addWidget(checkbox);
	return container;
}

static QCheckBox * get_checkbox_from_cell(QTableWidget * table, int row, int column)
{
	auto * container = table->cellWidget(row, column);
	if (!container)
		return nullptr;

	return container->findChild<QCheckBox *>();
}

sub_record_rules_view_t::sub_record_rules_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);

	m_table = new QTableWidget(0, column_count, this);
	m_table->setHorizontalHeader(new vertical_header_t(m_table));
	m_table->setHorizontalHeaderLabels(
	    { tr("Record"), tr("Sub"), tr("Ignore Conflict"), tr("Skip if Missing"), tr("Exclude from Merge"), "" });
	m_table->setAlternatingRowColors(true);
	m_table->verticalHeader()->setVisible(false);
	m_table->verticalHeader()->setDefaultSectionSize(row_height);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);

	m_table->horizontalHeader()->setSectionResizeMode(col_record, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(col_sub_record, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(col_ignore_conflict, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(col_skip_missing, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(col_exclude_merge, QHeaderView::ResizeToContents);
	m_table->horizontalHeader()->setSectionResizeMode(col_spacer, QHeaderView::Stretch);

	layout->addWidget(m_table, 1);

	auto * button_layout = new QHBoxLayout;

	m_add_button = new QPushButton(tr("Add"), this);
	m_add_button->setToolTip(tr("Add a new sub-record rule"));
	button_layout->addWidget(m_add_button);

	m_remove_button = new QPushButton(tr("Remove"), this);
	m_remove_button->setToolTip(tr("Remove the selected rule"));
	button_layout->addWidget(m_remove_button);

	button_layout->addStretch();
	layout->addLayout(button_layout);

	connect(m_add_button, &QPushButton::clicked, this, [this]() { add_row("CELL", "NAM0", false, false, false); });

	connect(m_remove_button, &QPushButton::clicked, this, &sub_record_rules_view_t::remove_selected_row);
}

void sub_record_rules_view_t::add_row(
    const QString & record,
    const QString & sub_record,
    bool ignore,
    bool exclude,
    bool skip)
{
	const int row = m_table->rowCount();
	m_table->insertRow(row);

	auto * record_item = new QTableWidgetItem(record);
	m_table->setItem(row, col_record, record_item);

	auto * sub_item = new QTableWidgetItem(sub_record);
	m_table->setItem(row, col_sub_record, sub_item);

	m_table->setCellWidget(row, col_ignore_conflict, make_centered_checkbox(m_table, ignore));
	m_table->setCellWidget(row, col_skip_missing, make_centered_checkbox(m_table, skip));
	m_table->setCellWidget(row, col_exclude_merge, make_centered_checkbox(m_table, exclude));
}

void sub_record_rules_view_t::remove_selected_row()
{
	const int row = m_table->currentRow();
	if (row >= 0)
		m_table->removeRow(row);
}

void sub_record_rules_view_t::load(const settings_store_t & settings)
{
	m_table->setRowCount(0);

	auto parse_field = [](const std::string & input) -> std::set<std::string>
	{
		std::set<std::string> result;
		size_t start = 0;

		while (start < input.size())
		{
			const auto comma = input.find(',', start);
			const auto end = (comma == std::string::npos) ? input.size() : comma;

			auto token_start = start;
			while (token_start < end && input[token_start] == ' ')
				++token_start;

			auto token_end = end;
			while (token_end > token_start && input[token_end - 1] == ' ')
				--token_end;

			if (token_end > token_start)
				result.insert(input.substr(token_start, token_end - token_start));

			start = (comma == std::string::npos) ? input.size() : comma + 1;
		}

		return result;
	};

	const auto ignore_set = parse_field(settings.sub_record_ignore_conflict());
	const auto exclude_set = parse_field(settings.sub_record_exclude_from_merge());
	const auto skip_set = parse_field(settings.sub_record_skip_if_missing());

	std::set<std::string> all_keys;
	all_keys.insert(ignore_set.begin(), ignore_set.end());
	all_keys.insert(exclude_set.begin(), exclude_set.end());
	all_keys.insert(skip_set.begin(), skip_set.end());

	for (const auto & key : all_keys)
	{
		const auto colon = key.find(':');
		if (colon == std::string::npos)
			continue;

		const auto record = QString::fromStdString(key.substr(0, colon));
		const auto sub_record = QString::fromStdString(key.substr(colon + 1));
		const bool ignore = ignore_set.count(key) > 0;
		const bool exclude = exclude_set.count(key) > 0;
		const bool skip = skip_set.count(key) > 0;

		add_row(record, sub_record, ignore, exclude, skip);
	}
}

void sub_record_rules_view_t::save(settings_store_t & settings) const
{
	std::string ignore_result;
	std::string exclude_result;
	std::string skip_result;

	for (int row = 0; row < m_table->rowCount(); ++row)
	{
		const auto * record_item = m_table->item(row, col_record);
		const auto * sub_item = m_table->item(row, col_sub_record);

		if (!record_item || !sub_item)
			continue;

		const auto record = record_item->text().trimmed();
		const auto sub_record = sub_item->text().trimmed();

		if (record.isEmpty() || sub_record.isEmpty())
			continue;

		const auto key = (record + ":" + sub_record).toStdString();

		const auto * ignore_check = get_checkbox_from_cell(m_table, row, col_ignore_conflict);
		const auto * exclude_check = get_checkbox_from_cell(m_table, row, col_exclude_merge);
		const auto * skip_check = get_checkbox_from_cell(m_table, row, col_skip_missing);

		if (ignore_check && ignore_check->isChecked())
		{
			if (!ignore_result.empty())
				ignore_result += ", ";

			ignore_result += key;
		}

		if (exclude_check && exclude_check->isChecked())
		{
			if (!exclude_result.empty())
				exclude_result += ", ";

			exclude_result += key;
		}

		if (skip_check && skip_check->isChecked())
		{
			if (!skip_result.empty())
				skip_result += ", ";

			skip_result += key;
		}
	}

	settings.set_sub_record_ignore_conflict(ignore_result);
	settings.set_sub_record_exclude_from_merge(exclude_result);
	settings.set_sub_record_skip_if_missing(skip_result);
}
