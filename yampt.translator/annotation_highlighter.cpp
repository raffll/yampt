#include "annotation_highlighter.hpp"
#include "annotation_manager.hpp"
#include <QTextCharFormat>
#include <algorithm>

annotation_highlighter_t::annotation_highlighter_t(QTextDocument * parent)
    : QSyntaxHighlighter(parent)
{}

void annotation_highlighter_t::set_annotation_manager(annotation_manager_t * manager)
{
	manager_ = manager;
}

void annotation_highlighter_t::set_record_type(tools_t::rec_type_t type)
{
	record_type_ = type;
}

void annotation_highlighter_t::set_annotations(const std::vector<annotation_t> & annotations)
{
	annotations_ = annotations;
	rehighlight();
}

void annotation_highlighter_t::highlightBlock(const QString & text)
{
	if (annotations_.empty())
		return;

	const int block_start = currentBlock().position();
	const int block_length = text.length();
	const int block_end = block_start + block_length;

	for (const auto & ann : annotations_)
	{
		const int ann_start = static_cast<int>(ann.start);
		const int ann_end = static_cast<int>(ann.end);

		if (ann_end <= block_start || ann_start >= block_end)
			continue;

		const int local_start = std::max(ann_start - block_start, 0);
		const int local_end = std::min(ann_end - block_start, block_length);
		const int length = local_end - local_start;

		if (length <= 0)
			continue;

		int overlap_count = 0;
		for (const auto & other : annotations_)
		{
			const int other_start = static_cast<int>(other.start);
			const int other_end = static_cast<int>(other.end);

			if (other_start <= ann_start && other_end > ann_start)
				++overlap_count;
		}

		if (overlap_count > 3)
			continue;

		QTextCharFormat fmt;

		switch (ann.kind)
		{
		case annotation_t::dial_topic:
			fmt.setBackground(QColor(70, 130, 200, 60));
			break;
		case annotation_t::glossary_term:
			fmt.setBackground(QColor(70, 180, 70, 60));
			break;
		default:
			continue;
		}

		fmt.setToolTip(QString::fromStdString(ann.new_text));
		setFormat(local_start, length, fmt);
	}
}
