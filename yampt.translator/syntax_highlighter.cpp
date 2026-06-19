#include "syntax_highlighter.hpp"
#include <algorithm>
#include <cctype>

static bool is_word_boundary(const std::string & text, size_t pos, size_t len)
{
	if (pos > 0)
	{
		char c = text[pos - 1];
		if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
			return false;
	}

	size_t after = pos + len;
	if (after < text.size())
	{
		char c = text[after];
		if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
			return false;
	}

	return true;
}

static bool ci_match_at(const std::string & text, size_t pos, const std::string & keyword)
{
	if (pos + keyword.size() > text.size())
		return false;

	for (size_t i = 0; i < keyword.size(); ++i)
	{
		char a = static_cast<char>(std::tolower(static_cast<unsigned char>(text[pos + i])));
		char b = static_cast<char>(std::tolower(static_cast<unsigned char>(keyword[i])));
		if (a != b)
			return false;
	}

	return true;
}

static bool overlaps_any(const std::vector<token_t> & tokens, size_t start, size_t end)
{
	for (const auto & t : tokens)
	{
		if (t.type == token_type_t::normal)
			continue;
		if (start < t.end && end > t.start)
			return true;
	}
	return false;
}

std::vector<token_t> syntax_highlighter_t::tokenize(const std::string & text, tools_t::rec_type_t type) const
{
	if (type == tools_t::rec_type_t::sctx)
		return tokenize_sctx(text);

	if (type == tools_t::rec_type_t::text)
		return tokenize_text(text);

	return { { 0, text.size(), token_type_t::normal } };
}

std::vector<token_t> syntax_highlighter_t::tokenize_sctx(const std::string & text) const
{
	static const std::string keywords[] = { "messagebox",   "say",          "journal",      "choice",
		                                    "addtopic",     "getpccell",    "positioncell", "showmap",
		                                    "centeroncell", "aifollowcell", "aiescortcell", "placeitemcell" };

	std::vector<token_t> tokens;

	size_t line_start = 0;
	while (line_start <= text.size())
	{
		size_t line_end = text.find('\n', line_start);
		if (line_end == std::string::npos)
			line_end = text.size();

		size_t first_non_space = line_start;
		while (first_non_space < line_end && (text[first_non_space] == ' ' || text[first_non_space] == '\t'))
			++first_non_space;

		if (first_non_space < line_end && text[first_non_space] == ';')
		{
			tokens.push_back({ line_start, line_end, token_type_t::mwscript_comment });
			line_start = line_end + 1;
			continue;
		}

		size_t pos = line_start;
		while (pos < line_end)
		{
			if (text[pos] == '"')
			{
				size_t close = text.find('"', pos + 1);
				if (close == std::string::npos || close > line_end)
					close = line_end;
				else
					close += 1;
				tokens.push_back({ pos, close, token_type_t::mwscript_string });
				pos = close;
				continue;
			}
			++pos;
		}

		for (const auto & kw : keywords)
		{
			size_t search_pos = line_start;
			while (search_pos < line_end)
			{
				size_t found = std::string::npos;
				for (size_t i = search_pos; i + kw.size() <= line_end; ++i)
				{
					if (ci_match_at(text, i, kw))
					{
						found = i;
						break;
					}
				}

				if (found == std::string::npos)
					break;

				if (is_word_boundary(text, found, kw.size()) && !overlaps_any(tokens, found, found + kw.size()))
				{
					tokens.push_back({ found, found + kw.size(), token_type_t::mwscript_function });
				}

				search_pos = found + kw.size();
			}
		}

		line_start = line_end + 1;
	}

	std::sort(tokens.begin(), tokens.end(), [](const token_t & a, const token_t & b) { return a.start < b.start; });

	std::vector<token_t> result;
	size_t pos = 0;
	for (const auto & t : tokens)
	{
		if (t.start > pos)
			result.push_back({ pos, t.start, token_type_t::normal });
		result.push_back(t);
		pos = t.end;
	}
	if (pos < text.size())
		result.push_back({ pos, text.size(), token_type_t::normal });

	return result;
}

std::vector<token_t> syntax_highlighter_t::tokenize_text(const std::string & text) const
{
	static const std::string tag_names[] = { "div", "font", "br", "p", "img", "b" };

	std::vector<token_t> tokens;

	size_t pos = 0;
	while (pos < text.size())
	{
		size_t open = text.find('<', pos);
		if (open == std::string::npos)
			break;

		size_t tag_start = open + 1;
		if (tag_start < text.size() && text[tag_start] == '/')
			++tag_start;

		bool matched = false;
		for (const auto & tag : tag_names)
		{
			if (!ci_match_at(text, tag_start, tag))
				continue;

			size_t after_name = tag_start + tag.size();
			if (after_name >= text.size())
				continue;

			char next = text[after_name];
			if (next != '>' && next != ' ' && next != '/' && next != '\t' && next != '\r' && next != '\n')
				continue;

			size_t close = text.find('>', open);
			if (close == std::string::npos)
				continue;

			tokens.push_back({ open, close + 1, token_type_t::html_tag });
			pos = close + 1;
			matched = true;
			break;
		}

		if (!matched)
			pos = open + 1;
	}

	std::sort(tokens.begin(), tokens.end(), [](const token_t & a, const token_t & b) { return a.start < b.start; });

	std::vector<token_t> result;
	pos = 0;
	for (const auto & t : tokens)
	{
		if (t.start > pos)
			result.push_back({ pos, t.start, token_type_t::normal });
		result.push_back(t);
		pos = t.end;
	}
	if (pos < text.size())
		result.push_back({ pos, text.size(), token_type_t::normal });

	return result;
}
