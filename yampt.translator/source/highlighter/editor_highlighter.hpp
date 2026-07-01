#pragma once

#include "script_tokenizer.hpp"
#include <utility/tools.hpp>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class spell_checker_t;

class editor_highlighter_t : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	explicit editor_highlighter_t(QTextDocument * parent = nullptr);

	void set_record_type(tools_t::rec_type_t type);
	void set_translation_mode(bool enabled);
	void set_spell_checker(spell_checker_t * checker);

protected:
	void highlightBlock(const QString & text) override;

private:
	void apply_syntax_tokens(const QString & text);
	void apply_forbidden_chars(const QString & text);
	void apply_hyperlink_prefix(const QString & text);
	void apply_spell_check(const QString & text);

	tools_t::rec_type_t m_record_type = tools_t::rec_type_t::unknown;
	bool m_is_translation = false;
	spell_checker_t * m_spell_checker = nullptr;
	script_tokenizer_t m_tokenizer;

	QTextCharFormat m_format_function;
	QTextCharFormat m_format_comment;
	QTextCharFormat m_format_string;
	QTextCharFormat m_format_html_tag;
	QTextCharFormat m_format_forbidden;
	QTextCharFormat m_format_hyperlink;
	QTextCharFormat m_format_misspelled;
};
