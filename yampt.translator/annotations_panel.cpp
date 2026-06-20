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

void annotations_panel_t::update_annotations(
    const std::vector<annotation_t> & annotations,
    const std::string & speaker_name,
    const std::string & gender,
    const std::string & enchantment)
{
	list_->clear();

	bool has_header = false;

	if (!enchantment.empty())
	{
		auto * item = new QListWidgetItem(QString::fromStdString("Enchantment: " + enchantment));
		list_->addItem(item);
		has_header = true;
	}

	if (!speaker_name.empty())
	{
		auto * item = new QListWidgetItem(QString::fromStdString("Speaker: " + speaker_name + " (" + gender + ")"));
		list_->addItem(item);
		has_header = true;
	}

	struct entry_t
	{
		std::string old_text;
		std::string new_text;
		std::string source;
	};

	auto deduplicate_and_sort = [](const std::vector<annotation_t> & annotations,
	                               annotation_t::kind_t kind) -> std::vector<entry_t>
	{
		std::set<std::string> seen;
		std::vector<entry_t> result;

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
		    [](const entry_t & lhs, const entry_t & rhs) { return lhs.old_text < rhs.old_text; });

		return result;
	};

	auto hyperlinks = deduplicate_and_sort(annotations, annotation_t::dial_topic);
	if (!hyperlinks.empty())
	{
		auto * header = new QListWidgetItem("--- Hyperlinks ---");
		header->setForeground(QColor(70, 130, 200));
		header->setFlags(Qt::NoItemFlags);
		list_->addItem(header);
	}

	for (const auto & e : hyperlinks)
	{
		QString display = QString::fromStdString(e.old_text + " \xe2\x86\x92 " + e.new_text);
		if (!e.source.empty())
		{
			auto sep = e.source.find_last_of("/\\");
			auto name = (sep != std::string::npos) ? e.source.substr(sep + 1) : e.source;
			display += QString::fromStdString("  [" + name + "]");
		}
		auto * item = new QListWidgetItem(display);
		item->setData(Qt::UserRole, QString::fromStdString(e.new_text));
		list_->addItem(item);
	}

	auto glossary = deduplicate_and_sort(annotations, annotation_t::glossary_term);
	if (!glossary.empty())
	{
		auto * header = new QListWidgetItem("--- Glossary ---");
		header->setForeground(QColor(50, 150, 50));
		header->setFlags(Qt::NoItemFlags);
		list_->addItem(header);
	}

	for (const auto & e : glossary)
	{
		QString display = QString::fromStdString(e.old_text + " \xe2\x86\x92 " + e.new_text);
		if (!e.source.empty())
		{
			auto sep = e.source.find_last_of("/\\");
			auto name = (sep != std::string::npos) ? e.source.substr(sep + 1) : e.source;
			display += QString::fromStdString("  [" + name + "]");
		}
		auto * item = new QListWidgetItem(display);
		item->setData(Qt::UserRole, QString::fromStdString(e.new_text));
		list_->addItem(item);
	}
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
