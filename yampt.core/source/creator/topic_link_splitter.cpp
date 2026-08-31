#include "creator/topic_link_splitter.hpp"

namespace
{
constexpr char link_open = '@';
constexpr char link_close = '#';

link_segment_t make_plain_segment(const std::string & text)
{
	link_segment_t segment;
	segment.is_link = false;
	segment.plain_text = text;

	return segment;
}

link_segment_t make_link_segment(const std::string & inner)
{
	const auto phrase_end = inner.find_last_not_of('*');

	link_segment_t segment;
	segment.is_link = true;

	if (phrase_end == std::string::npos)
	{
		segment.pseudo_asterisks = inner;
		return segment;
	}

	segment.inner_phrase = inner.substr(0, phrase_end + 1);
	segment.pseudo_asterisks = inner.substr(phrase_end + 1);

	return segment;
}
}

namespace topic_link_splitter {

std::vector<link_segment_t> split(const std::string & line)
{
	std::vector<link_segment_t> segments;

	std::size_t scan_pos = 0;
	while (scan_pos < line.size())
	{
		const auto open_pos = line.find(link_open, scan_pos);
		if (open_pos == std::string::npos)
		{
			segments.push_back(make_plain_segment(line.substr(scan_pos)));
			break;
		}

		const auto close_pos = line.find(link_close, open_pos + 1);
		if (close_pos == std::string::npos)
		{
			segments.push_back(make_plain_segment(line.substr(scan_pos)));
			break;
		}

		if (open_pos > scan_pos)
			segments.push_back(make_plain_segment(line.substr(scan_pos, open_pos - scan_pos)));

		const auto inner = line.substr(open_pos + 1, close_pos - (open_pos + 1));
		segments.push_back(make_link_segment(inner));

		scan_pos = close_pos + 1;
	}

	return segments;
}

std::string reassemble(const std::vector<link_segment_t> & segments)
{
	std::string result;

	for (const auto & segment : segments)
	{
		if (!segment.is_link)
		{
			result.append(segment.plain_text);
			continue;
		}

		result.push_back(link_open);
		result.append(segment.inner_phrase);
		result.append(segment.pseudo_asterisks);
		result.push_back(link_close);
	}

	return result;
}

} // namespace topic_link_splitter
