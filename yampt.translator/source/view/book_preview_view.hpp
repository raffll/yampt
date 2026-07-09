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
	void set_script(const std::string & original_script, const std::string & translated_script);
	void clear();

private:
	QString prepare_html(const std::string & html) const;
	QString highlight_script(const std::string & script) const;

	QSplitter * m_splitter = nullptr;
	QTextBrowser * m_original_browser = nullptr;
	QTextBrowser * m_translation_browser = nullptr;
};
