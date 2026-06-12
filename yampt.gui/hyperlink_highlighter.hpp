#pragma once

#include <QSyntaxHighlighter>
#include <string>
#include <vector>

class hyperlink_highlighter_t : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	explicit hyperlink_highlighter_t(QTextDocument * parent = nullptr);

	void set_terms(const std::vector<std::string> & translated_terms);

protected:
	void highlightBlock(const QString & text) override;

private:
	std::vector<std::string> terms_;
	QTextCharFormat format_;
};
