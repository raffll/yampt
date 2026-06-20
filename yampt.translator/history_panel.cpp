#include "history_panel.hpp"
#include "history_manager.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

history_panel_t::history_panel_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);

	auto * scroll_area = new QScrollArea(this);
	scroll_area->setWidgetResizable(true);
	scroll_area->setFrameShape(QFrame::NoFrame);

	scroll_content_ = new QWidget(scroll_area);
	entries_layout_ = new QVBoxLayout(scroll_content_);
	entries_layout_->setContentsMargins(4, 4, 4, 4);
	entries_layout_->setSpacing(4);
	entries_layout_->addStretch();

	scroll_area->setWidget(scroll_content_);
	layout->addWidget(scroll_area);
}

void history_panel_t::update_history(const std::vector<history_entry_t> & entries, bool allow_revert)
{
	clear();

	for (size_t i = entries.size(); i > 0; --i)
	{
		const auto & entry = entries[i - 1];
		size_t history_index = i - 1;

		auto * row = new QWidget(scroll_content_);
		auto * row_layout = new QVBoxLayout(row);
		row_layout->setContentsMargins(2, 2, 2, 2);
		row_layout->setSpacing(2);

		auto timestamp_str = QString::fromStdString(entry.timestamp);
		auto value_str = QString::fromStdString(entry.value);
		if (value_str.length() > 80)
			value_str = value_str.left(80) + "...";

		auto * label = new QLabel(QString("[%1] %2").arg(timestamp_str, value_str), row);
		label->setWordWrap(true);
		row_layout->addWidget(label);

		if (allow_revert)
		{
			auto * button_layout = new QHBoxLayout();
			button_layout->setContentsMargins(0, 0, 0, 0);

			auto * revert_btn = new QPushButton(tr("Revert"), row);
			connect(
			    revert_btn,
			    &QPushButton::clicked,
			    this,
			    [this, history_index]() { emit revert_requested(history_index); });

			button_layout->addWidget(revert_btn);
			button_layout->addStretch();
			row_layout->addLayout(button_layout);
		}

		entries_layout_->insertWidget(entries_layout_->count() - 1, row);
	}
}

void history_panel_t::clear()
{
	while (entries_layout_->count() > 1)
	{
		auto * item = entries_layout_->takeAt(0);
		if (item->widget())
		{
			delete item->widget();
		}
		delete item;
	}
}
