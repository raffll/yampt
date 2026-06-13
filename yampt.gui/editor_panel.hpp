#pragma once

#include <QTextEdit>
#include <QWidget>
#include <string>

class QLabel;
class QPushButton;
class QSplitter;
class QTextEdit;

class editor_panel_t : public QWidget
{
    Q_OBJECT

public:
    explicit editor_panel_t(QWidget * parent = nullptr);

    QTextEdit * original_view() const;
    QTextEdit * translation_editor() const;

    void set_adapted_from(const std::string & text);
    void highlight_adapted_diff(const std::string & new_text, const std::string & adapted_from, bool use_original = false);
    void clear_adapted_from();

    void set_split_ratio(double ratio);
    double get_split_ratio() const;

signals:
    void text_changed();
    void apply_clicked();

private:
    QSplitter * splitter_ = nullptr;
    QTextEdit * original_view_ = nullptr;
    QTextEdit * adapted_from_view_ = nullptr;
    QLabel * adapted_from_label_ = nullptr;
    QTextEdit * translation_editor_ = nullptr;
    QLabel * original_label_ = nullptr;
    QLabel * translation_label_ = nullptr;
    QPushButton * apply_button_ = nullptr;
};
