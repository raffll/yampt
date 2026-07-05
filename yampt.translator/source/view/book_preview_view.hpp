#pragma once

#include <string>
#include <QWidget>

class QSplitter;
class QTextBrowser;

class book_preview_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit book_preview_view_t(QWidget * parent = nullptr);

	void set_html(const std::string & original_html, const std::string & translation_html);
	void clear();

private:
	QString prepare_html(const std::string & html) const;

	QSplitter * m_splitter = nullptr;
	QTextBrowser * m_original_browser = nullptr;
	QTextBrowser * m_translation_browser = nullptr;
};
