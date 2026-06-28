#include "annotations_panel.hpp"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>
#include <set>

annotations_panel_t::annotations_panel_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);

	auto * toolbar = new QHBoxLayout();
	toolbar->setContentsMargins(0, 0, 0, 0);
	rebuild_btn_ = new QPushButton("Rebuild", this);
	rebuild_btn_->setToolTip("Rebuild annotations from all loaded dicts");
	rebuild_btn_->setFixedHeight(22);
	toolbar->addWidget(rebuild_btn_);
	toolbar->addStretch();
	layout->addLayout(toolbar);

	list_ = new QListWidget(this);
	layout->addWidget(list_);

	connect(list_, &QListWidget::itemClicked, this, &annotations_panel_t::on_item_clicked);
	connect(rebuild_btn_, &QPushButton::clicked, this, &annotations_panel_t::rebuild_requested);
}

struct annotation_entry_t
{
	std::string old_text;
	std::string new_text;
	std::string source;
};

static std::vector<annotation_entry_t> deduplicate_and_sort(
    const std::vector<annotation_t> & annotations,
    annotation_t::kind_t kind)
{
	std::set<std::string> seen;
	std::vector<annotation_entry_t> result;

	for (const auto & a : annotations)
	{
		if (a.kind != kind)
			continue;

		auto key = a.old_text + "\x01" + a.new_text + "\x01" + a.source;
		if (seen.count(key))
			continue;

		seen.insert(key);
		result.push_back({ a.old_text, a.new_text, a.source });
	}

	std::sort(
	    result.begin(),
	    result.end(),
	    [](const annotation_entry_t & lhs, const annotation_entry_t & rhs) { return lhs.old_text < rhs.old_text; });

	return result;
}

static QString format_annotation_display(const annotation_entry_t & entry)
{
	QString display = QString::fromStdString(entry.old_text + " \xe2\x86\x92 " + entry.new_text);
	if (entry.source.empty())
		return display;

	auto sep = entry.source.find_last_of("/\\");
	auto name = (sep != std::string::npos) ? entry.source.substr(sep + 1) : entry.source;
	display += QString::fromStdString("  [" + name + "]");
	return display;
}

static void add_annotation_section(
    QListWidget * list,
    const std::vector<annotation_entry_t> & entries,
    const char * header_text,
    const QColor & header_color)
{
	if (entries.empty())
		return;

	auto * header = new QListWidgetItem(header_text);
	header->setForeground(header_color);
	header->setFlags(Qt::NoItemFlags);
	list->addItem(header);

	for (const auto & entry : entries)
	{
		auto * item = new QListWidgetItem(format_annotation_display(entry));
		item->setData(Qt::UserRole, QString::fromStdString(entry.new_text));
		list->addItem(item);
	}
}

void annotations_panel_t::update_annotations(
    const std::vector<annotation_t> & annotations,
    const std::string & speaker_name,
    const std::string & gender,
    const std::string & enchantment)
{
	list_->clear();

	if (!enchantment.empty())
	{
		auto * item = new QListWidgetItem(QString::fromStdString("Enchantment: " + enchantment));
		list_->addItem(item);
	}

	if (!speaker_name.empty())
	{
		auto * item = new QListWidgetItem(QString::fromStdString("Speaker: " + speaker_name + " (" + gender + ")"));
		list_->addItem(item);
	}

	const auto hyperlinks = deduplicate_and_sort(annotations, annotation_t::dial_topic);
	add_annotation_section(list_, hyperlinks, "--- Hyperlinks ---", QColor(70, 130, 200));

	const auto glossary = deduplicate_and_sort(annotations, annotation_t::glossary_term);
	add_annotation_section(list_, glossary, "--- Glossary ---", QColor(50, 150, 50));
}

void annotations_panel_t::on_item_clicked(QListWidgetItem * item)
{
	if (!item)
		return;

	auto new_text = item->data(Qt::UserRole).toString();
	if (new_text.isEmpty())
		return;

	QApplication::clipboard()->setText(new_text);
	emit annotation_clicked(new_text.toStdString());
}

void annotations_panel_t::clear()
{
	list_->clear();
}
