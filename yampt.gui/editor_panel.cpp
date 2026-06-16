#include "editor_panel.hpp"
#include "line_number_gutter.hpp"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QSplitter>
#include <QString>
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
    original_view_ = new editor_text_edit_t(left_widget);
    original_view_->setReadOnly(true);
    auto palette = original_view_->palette();
    palette.setColor(QPalette::Base, QColor(245, 245, 245));
    original_view_->setPalette(palette);

    auto * original_container = new QWidget(left_widget);
    auto * original_hlayout = new QHBoxLayout(original_container);
    original_hlayout->setContentsMargins(0, 0, 0, 0);
    original_hlayout->setSpacing(0);
    original_hlayout->addWidget(new line_number_gutter_t(original_view_, original_container));
    original_hlayout->addWidget(original_view_);

    adapted_from_label_ = new QLabel("Adapted from", left_widget);
    adapted_from_view_ = new editor_text_edit_t(left_widget);
    adapted_from_view_->setReadOnly(true);
    auto adapted_palette = adapted_from_view_->palette();
    adapted_palette.setColor(QPalette::Base, QColor(240, 235, 250));
    adapted_from_view_->setPalette(adapted_palette);

    adapted_from_container_ = new QWidget(left_widget);
    auto * adapted_hlayout = new QHBoxLayout(adapted_from_container_);
    adapted_hlayout->setContentsMargins(0, 0, 0, 0);
    adapted_hlayout->setSpacing(0);
    adapted_hlayout->addWidget(new line_number_gutter_t(adapted_from_view_, adapted_from_container_));
    adapted_hlayout->addWidget(adapted_from_view_);

    adapted_from_label_->setVisible(false);
    adapted_from_container_->setVisible(false);

    left_layout->addWidget(original_label_);
    left_layout->addWidget(original_container);
    left_layout->addWidget(adapted_from_label_);
    left_layout->addWidget(adapted_from_container_);

    auto * right_widget = new QWidget(splitter_);
    auto * right_layout = new QVBoxLayout(right_widget);
    right_layout->setContentsMargins(0, 0, 0, 0);
    translation_label_ = new QLabel("Translation", right_widget);
    translation_editor_ = new editor_text_edit_t(right_widget);
    apply_button_ = new QPushButton("Apply", right_widget);

    auto * translation_container = new QWidget(right_widget);
    auto * translation_hlayout = new QHBoxLayout(translation_container);
    translation_hlayout->setContentsMargins(0, 0, 0, 0);
    translation_hlayout->setSpacing(0);
    translation_hlayout->addWidget(new line_number_gutter_t(translation_editor_, translation_container));
    translation_hlayout->addWidget(translation_editor_);

    right_layout->addWidget(translation_label_);
    right_layout->addWidget(translation_container);
    right_layout->addWidget(apply_button_);

    QFont editor_font("Segoe UI", 10);
    original_view_->setFont(editor_font);
    original_container->setFont(editor_font);
    adapted_from_view_->setFont(editor_font);
    adapted_from_container_->setFont(editor_font);
    translation_editor_->setFont(editor_font);
    translation_container->setFont(editor_font);

    splitter_->addWidget(left_widget);
    splitter_->addWidget(right_widget);
    splitter_->setSizes({500, 500});

    layout->addWidget(splitter_);

    connect(translation_editor_, &QPlainTextEdit::textChanged, this, &editor_panel_t::text_changed);
    connect(apply_button_, &QPushButton::clicked, this, &editor_panel_t::apply_clicked);
}

editor_text_edit_t * editor_panel_t::original_view() const
{
    return original_view_;
}

editor_text_edit_t * editor_panel_t::adapted_from_view() const
{
    return adapted_from_view_;
}

editor_text_edit_t * editor_panel_t::translation_editor() const
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
    adapted_from_container_->setVisible(true);
}

QList<QTextEdit::ExtraSelection> editor_panel_t::highlight_adapted_diff(const std::string & new_text, const std::string & adapted_from, bool /*use_original*/)
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

    return from_selections;
}

void editor_panel_t::clear_adapted_from()
{
    adapted_from_view_->clear();
    adapted_from_label_->setVisible(false);
    adapted_from_container_->setVisible(false);
}

void editor_panel_t::load_script_entry(const std::string & old_text, const std::string & new_text)
{
    script_template_t tmpl;
    tmpl.full_line = old_text;

    size_t pos = 0;
    while (pos < old_text.size())
    {
        auto i = old_text.find('"', pos);
        if (i == std::string::npos)
            break;

        auto j = old_text.find('"', i + 1);
        if (j == std::string::npos)
            break;

        tmpl.quote_starts.push_back(i);
        tmpl.quote_ends.push_back(j);
        pos = j + 1;
    }

    if (tmpl.quote_starts.empty())
    {
        script_template_ = std::nullopt;
        translation_editor_->setPlainText(QString::fromStdString(new_text));
        return;
    }

    script_template_ = tmpl;

    std::vector<std::string> original_extracted;
    for (size_t k = 0; k < tmpl.quote_starts.size(); ++k)
        original_extracted.push_back(old_text.substr(tmpl.quote_starts[k] + 1, tmpl.quote_ends[k] - tmpl.quote_starts[k] - 1));

    QString original_stripped;
    for (size_t k = 0; k < original_extracted.size(); ++k)
    {
        if (k > 0)
            original_stripped += '\n';
        original_stripped += QString::fromStdString(original_extracted[k]);
    }

    original_view_->setPlainText(original_stripped);

    std::vector<std::string> extracted;
    pos = 0;
    while (pos < new_text.size())
    {
        auto i = new_text.find('"', pos);
        if (i == std::string::npos)
            break;

        auto j = new_text.find('"', i + 1);
        if (j == std::string::npos)
            break;

        extracted.push_back(new_text.substr(i + 1, j - i - 1));
        pos = j + 1;
    }

    QString stripped;
    for (size_t k = 0; k < extracted.size(); ++k)
    {
        if (k > 0)
            stripped += '\n';
        stripped += QString::fromStdString(extracted[k]);
    }

    translation_editor_->setPlainText(stripped);
}

std::string editor_panel_t::reconstruct_script_line() const
{
    if (!script_template_)
        return translation_editor_->toPlainText().toStdString();

    const auto & tmpl = *script_template_;
    const auto lines = translation_editor_->toPlainText().split('\n');
    const size_t slot_count = tmpl.quote_starts.size();

    std::string result = tmpl.full_line;

    for (size_t k = slot_count; k > 0; --k)
    {
        const size_t idx = k - 1;
        std::string replacement;
        if (idx < static_cast<size_t>(lines.size()))
            replacement = lines[static_cast<int>(idx)].toStdString();

        size_t start = tmpl.quote_starts[idx] + 1;
        size_t end = tmpl.quote_ends[idx];
        result.replace(start, end - start, replacement);
    }

    return result;
}

bool editor_panel_t::has_script_template() const
{
    return script_template_.has_value();
}

size_t editor_panel_t::script_slot_count() const
{
    if (!script_template_)
        return 0;

    return script_template_->quote_starts.size();
}

void editor_panel_t::clear_script_template()
{
    script_template_ = std::nullopt;
}
