#pragma once

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class spell_checker_t;

class spell_check_highlighter_t : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	explicit spell_check_highlighter_t(QTextDocument * parent = nullptr);

	void set_spell_checker(spell_checker_t * checker);

protected:
	void highlightBlock(const QString & text) override;

private:
	spell_checker_t * checker_ = nullptr;
	QTextCharFormat misspelled_format_;
};
