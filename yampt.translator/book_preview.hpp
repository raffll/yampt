#pragma once

#include <QWidget>

#include <string>

class QSplitter;
class QTextBrowser;

class book_preview_t : public QWidget
{
	Q_OBJECT

public:
	explicit book_preview_t(QWidget * parent = nullptr);

	void set_html(const std::string & original_html, const std::string & translation_html);
	void clear();

private:
	QString prepare_html(const std::string & html) const;

	QSplitter * splitter_ = nullptr;
	QTextBrowser * original_browser_ = nullptr;
	QTextBrowser * translation_browser_ = nullptr;
};
