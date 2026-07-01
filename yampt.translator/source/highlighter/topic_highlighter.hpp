#pragma once

#include <string>
#include <vector>
#include <QSyntaxHighlighter>

class topic_highlighter_t : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	explicit topic_highlighter_t(QTextDocument * parent = nullptr);

	void set_terms(const std::vector<std::string> & translated_terms);

protected:
	void highlightBlock(const QString & text) override;

private:
	std::vector<std::string> m_terms;
	QTextCharFormat m_format;
};
