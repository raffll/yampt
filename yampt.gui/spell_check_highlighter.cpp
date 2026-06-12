#include "spell_check_highlighter.hpp"
#include "spell_checker.hpp"

spell_check_highlighter_t::spell_check_highlighter_t(QTextDocument * parent)
	: QSyntaxHighlighter(parent)
{
	misspelled_format_.setUnderlineStyle(QTextCharFormat::WaveUnderline);
	misspelled_format_.setUnderlineColor(QColor(220, 50, 50));
}

void spell_check_highlighter_t::set_spell_checker(spell_checker_t * checker)
{
	checker_ = checker;

	if (document())
		rehighlight();
}

void spell_check_highlighter_t::highlightBlock(const QString & text)
{
	if (!checker_ || !checker_->is_loaded())
		return;

	const auto text_str = text.toStdString();
	const auto matches = checker_->find_misspelled(text_str);

	for (const auto & match : matches)
		setFormat(static_cast<int>(match.start), static_cast<int>(match.end - match.start), misspelled_format_);
}
