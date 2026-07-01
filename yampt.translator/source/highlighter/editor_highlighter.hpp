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
	void apply_spell_check(const QString & text);

	tools_t::rec_type_t record_type_ = tools_t::rec_type_t::unknown;
	bool is_translation_ = false;
	spell_checker_t * spell_checker_ = nullptr;
	script_tokenizer_t tokenizer_;

	QTextCharFormat format_function_;
	QTextCharFormat format_comment_;
	QTextCharFormat format_string_;
	QTextCharFormat format_html_tag_;
	QTextCharFormat format_forbidden_;
	QTextCharFormat format_misspelled_;
};
