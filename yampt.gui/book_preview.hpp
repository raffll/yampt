#pragma once

#include <QWidget>

#include <string>

class QTextBrowser;

class book_preview_t : public QWidget
{
    Q_OBJECT

public:
    explicit book_preview_t(QWidget * parent = nullptr);

    void set_html(const std::string & html);
    void clear();

private:
    QTextBrowser * browser_ = nullptr;
};
