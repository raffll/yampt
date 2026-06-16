#pragma once

#include "../yampt/tools.hpp"
#include "syntax_highlighter.hpp"

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class spell_checker_t;

class composite_highlighter_t : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	explicit composite_highlighter_t(QTextDocument * parent = nullptr);

	void set_record_type(tools_t::rec_type_t type);
	void set_translation_mode(bool enabled);
	void set_spell_checker(spell_checker_t * checker);

protected:
	void highlightBlock(const QString & text) override;

private:
	tools_t::rec_type_t record_type_ = tools_t::rec_type_t::unknown;
	bool is_translation_ = false;
	spell_checker_t * spell_checker_ = nullptr;
	syntax_highlighter_t tokenizer_;

	QTextCharFormat format_function_;
	QTextCharFormat format_comment_;
	QTextCharFormat format_string_;
	QTextCharFormat format_html_tag_;
	QTextCharFormat format_forbidden_;
	QTextCharFormat format_misspelled_;
};
