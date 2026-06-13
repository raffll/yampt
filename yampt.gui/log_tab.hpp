#pragma once

#include <QWidget>
#include <string>

class QPlainTextEdit;
class QPushButton;

class log_tab_t : public QWidget
{
    Q_OBJECT

public:
    explicit log_tab_t(QWidget * parent = nullptr);

    void append_log(const std::string & operation_name, const std::string & log_text);
    void clear();

private:
    QPlainTextEdit * text_edit_ = nullptr;
    QPushButton * clear_button_ = nullptr;
};
