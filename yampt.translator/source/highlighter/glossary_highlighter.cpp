#include "glossary_highlighter.hpp"
#include "../editor/glossary.hpp"
#include <algorithm>
#include <QTextCharFormat>

glossary_highlighter_t::glossary_highlighter_t(QTextDocument * parent)
    : QSyntaxHighlighter(parent)
{}

void glossary_highlighter_t::set_annotation_manager(glossary_t * manager)
{
	manager_ = manager;
}

void glossary_highlighter_t::set_record_type(tools_t::rec_type_t type)
{
	record_type_ = type;
}

void glossary_highlighter_t::set_annotations(const std::vector<annotation_t> & annotations)
{
	annotations_ = annotations;
	rehighlight();
}

static int count_overlaps(const std::vector<annotation_t> & annotations, int target_start)
{
	int overlap_count = 0;
	for (const auto & other : annotations)
	{
		const int other_start = static_cast<int>(other.start);
		const int other_end = static_cast<int>(other.end);

		if (other_start <= target_start && other_end > target_start)
			++overlap_count;
	}

	return overlap_count;
}

static QTextCharFormat format_for_annotation(const annotation_t & annotation)
{
	QTextCharFormat format;

	switch (annotation.kind)
	{
	case annotation_t::dial_topic:
		format.setBackground(QColor(70, 130, 200, 60));
		break;
	case annotation_t::glossary_term:
		format.setBackground(QColor(70, 180, 70, 60));
		break;
	default:
		return {};
	}

	format.setToolTip(QString::fromStdString(annotation.new_text));
	return format;
}

void glossary_highlighter_t::highlightBlock(const QString & text)
{
	if (annotations_.empty())
		return;

	const int block_start = currentBlock().position();
	const int block_length = text.length();
	const int block_end = block_start + block_length;

	for (const auto & annotation : annotations_)
	{
		const int ann_start = static_cast<int>(annotation.start);
		const int ann_end = static_cast<int>(annotation.end);

		if (ann_end <= block_start || ann_start >= block_end)
			continue;

		const int local_start = std::max(ann_start - block_start, 0);
		const int local_end = std::min(ann_end - block_start, block_length);
		const int length = local_end - local_start;

		if (length <= 0)
			continue;

		if (count_overlaps(annotations_, ann_start) > 3)
			continue;

		const auto & format = format_for_annotation(annotation);
		if (format.background().style() == Qt::NoBrush)
			continue;

		setFormat(local_start, length, format);
	}
}
