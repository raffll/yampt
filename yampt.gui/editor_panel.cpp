#include "editor_panel.hpp"

#include <QFont>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QSplitter>
#include <QTextEdit>
#include <QVBoxLayout>

#include <algorithm>

editor_panel_t::editor_panel_t(QWidget * parent)
    : QWidget(parent)
{
    auto * layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    splitter_ = new QSplitter(Qt::Horizontal, this);

    auto * left_widget = new QWidget(splitter_);
    auto * left_layout = new QVBoxLayout(left_widget);
    left_layout->setContentsMargins(0, 0, 0, 0);
    original_label_ = new QLabel("Original", left_widget);
    original_view_ = new QTextEdit(left_widget);
    original_view_->setReadOnly(true);
    auto palette = original_view_->palette();
    palette.setColor(QPalette::Base, QColor(245, 245, 245));
    original_view_->setPalette(palette);

    adapted_from_label_ = new QLabel("Adapted from", left_widget);
    adapted_from_view_ = new QTextEdit(left_widget);
    adapted_from_view_->setReadOnly(true);
    auto adapted_palette = adapted_from_view_->palette();
    adapted_palette.setColor(QPalette::Base, QColor(240, 235, 250));
    adapted_from_view_->setPalette(adapted_palette);
    adapted_from_label_->setVisible(false);
    adapted_from_view_->setVisible(false);

    left_layout->addWidget(original_label_);
    left_layout->addWidget(original_view_);
    left_layout->addWidget(adapted_from_label_);
    left_layout->addWidget(adapted_from_view_);

    auto * right_widget = new QWidget(splitter_);
    auto * right_layout = new QVBoxLayout(right_widget);
    right_layout->setContentsMargins(0, 0, 0, 0);
    translation_label_ = new QLabel("Translation", right_widget);
    translation_editor_ = new QTextEdit(right_widget);
    apply_button_ = new QPushButton("Apply", right_widget);
    right_layout->addWidget(translation_label_);
    right_layout->addWidget(translation_editor_);
    right_layout->addWidget(apply_button_);

    QFont editor_font("Segoe UI", 10);
    original_view_->setFont(editor_font);
    adapted_from_view_->setFont(editor_font);
    translation_editor_->setFont(editor_font);

    splitter_->addWidget(left_widget);
    splitter_->addWidget(right_widget);
    splitter_->setSizes({500, 500});

    layout->addWidget(splitter_);

    connect(translation_editor_, &QTextEdit::textChanged, this, &editor_panel_t::text_changed);
    connect(apply_button_, &QPushButton::clicked, this, &editor_panel_t::apply_clicked);
}

QTextEdit * editor_panel_t::original_view() const
{
    return original_view_;
}

QTextEdit * editor_panel_t::translation_editor() const
{
    return translation_editor_;
}

void editor_panel_t::set_split_ratio(double ratio)
{
    ratio = std::clamp(ratio, 0.2, 0.8);
    const int total = splitter_->width();
    const int left = static_cast<int>(total * ratio);
    splitter_->setSizes({left, total - left});
}

double editor_panel_t::get_split_ratio() const
{
    const auto sizes = splitter_->sizes();
    const int total = sizes[0] + sizes[1];
    if (total == 0)
        return 0.5;

    return static_cast<double>(sizes[0]) / total;
}

void editor_panel_t::set_adapted_from(const std::string & text)
{
    adapted_from_view_->setPlainText(QString::fromStdString(text));
    adapted_from_label_->setVisible(true);
    adapted_from_view_->setVisible(true);
}

void editor_panel_t::highlight_adapted_diff(const std::string & new_text, const std::string & adapted_from, bool /*use_original*/)
{
    const auto new_str = QString::fromStdString(new_text);
    const auto from_str = QString::fromStdString(adapted_from);

    int prefix = 0;
    int max_prefix = std::min(new_str.length(), from_str.length());
    while (prefix < max_prefix && new_str[prefix] == from_str[prefix])
        ++prefix;

    QList<QTextEdit::ExtraSelection> from_selections;
    if (prefix < from_str.length())
    {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(QColor(255, 220, 150));
        sel.cursor = adapted_from_view_->textCursor();
        sel.cursor.setPosition(prefix);
        sel.cursor.setPosition(from_str.length(), QTextCursor::KeepAnchor);
        from_selections.append(sel);
    }

    adapted_from_view_->setExtraSelections(from_selections);
}

void editor_panel_t::clear_adapted_from()
{
    adapted_from_view_->clear();
    adapted_from_label_->setVisible(false);
    adapted_from_view_->setVisible(false);
}
