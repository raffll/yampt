#include "theme_system.hpp"
#include "conflict_types.hpp"
#include <QApplication>
#include <QPalette>

theme_system_t & theme_system_t::instance()
{
	static theme_system_t instance;
	return instance;
}

theme_t theme_system_t::active_theme() const
{
	return m_active_theme;
}

void theme_system_t::set_theme(theme_t theme)
{
	if (m_active_theme == theme)
		return;

	m_active_theme = theme;
	emit theme_changed(theme);
}

QColor theme_system_t::get_color(color_name_t name) const
{
	return get_color(name, m_active_theme);
}

QColor theme_system_t::get_color(color_name_t name, theme_t theme) const
{
	const auto index = static_cast<size_t>(name);
	if (index >= static_cast<size_t>(color_name_t::color_name_count))
		return QColor(255, 0, 255);

	const auto & rgb = (theme == theme_t::dark) ? dark_palette[index] : light_palette[index];

	return QColor(rgb.red, rgb.green, rgb.blue, rgb.alpha);
}

QColor theme_system_t::get_status_color(status_t status) const
{
	return get_status_color(status, m_active_theme);
}

QColor theme_system_t::get_status_color(status_t status, theme_t theme) const
{
	switch (status)
	{
	case status_t::translated:
		return get_color(color_name_t::status_translated, theme);
	case status_t::untranslated:
		return get_color(color_name_t::status_untranslated, theme);
	case status_t::missing:
		return get_color(color_name_t::status_missing, theme);
	case status_t::duplicate:
		return get_color(color_name_t::status_duplicate, theme);
	case status_t::mismatch:
		return get_color(color_name_t::status_mismatch, theme);
	case status_t::error:
		return get_color(color_name_t::status_error, theme);
	case status_t::reused:
		return get_color(color_name_t::status_reused, theme);
	case status_t::adapted:
		return get_color(color_name_t::status_adapted, theme);
	case status_t::changed:
		return get_color(color_name_t::status_changed, theme);
	case status_t::outdated:
		return get_color(color_name_t::status_outdated, theme);
	case status_t::in_progress:
		return get_color(color_name_t::status_in_progress, theme);
	case status_t::model:
		return get_color(color_name_t::status_model, theme);
	case status_t::propagated:
		return get_color(color_name_t::status_propagated, theme);
	case status_t::heuristic:
		return get_color(color_name_t::status_heuristic, theme);
	case status_t::to_verify:
		return get_color(color_name_t::status_to_verify, theme);
	case status_t::ambiguous:
		return get_color(color_name_t::status_ambiguous, theme);
	}

	return get_color(color_name_t::status_error, theme);
}

QColor theme_system_t::conflict_all_background(conflict_all_t value) const
{
	if (value < conflict_all_t::no_conflict)
	{
		return (m_active_theme == theme_t::dark) ? QColor(30, 30, 30) : QColor(255, 255, 255);
	}

	color_name_t raw_name = color_name_t::conflict_all_no_conflict_raw;

	switch (value)
	{
	case conflict_all_t::no_conflict:
		raw_name = color_name_t::conflict_all_no_conflict_raw;
		break;
	case conflict_all_t::override_benign:
		raw_name = color_name_t::conflict_all_override_benign_raw;
		break;
	case conflict_all_t::conflict:
		raw_name = color_name_t::conflict_all_conflict_raw;
		break;
	default:
		break;
	}

	const auto raw_color = get_color(raw_name);

	if (m_active_theme == theme_t::dark)
		return darker_hsl(raw_color, 0.70);

	return lighter_hsl(raw_color, 0.85);
}

QColor theme_system_t::conflict_this_foreground(conflict_this_t value) const
{
	switch (value)
	{
	case conflict_this_t::master:
		return get_color(color_name_t::conflict_this_master);
	case conflict_this_t::identical_to_master:
		return get_color(color_name_t::conflict_this_identical);
	case conflict_this_t::override_wins:
		return get_color(color_name_t::conflict_this_override_wins);
	case conflict_this_t::conflict_wins:
		return get_color(color_name_t::conflict_this_conflict_wins);
	case conflict_this_t::conflict_loses:
		return get_color(color_name_t::conflict_this_conflict_loses);
	case conflict_this_t::deleted:
		return get_color(color_name_t::conflict_this_deleted);
	default:
		return (m_active_theme == theme_t::dark) ? QColor(220, 220, 220) : QColor(0, 0, 0);
	}
}

void theme_system_t::apply_to_application()
{
	apply_palette();
	apply_stylesheet();
}

void theme_system_t::apply_palette() const
{
	QPalette palette;

	if (m_active_theme == theme_t::light)
	{
		palette.setColor(QPalette::Window, QColor(240, 240, 240));
		palette.setColor(QPalette::WindowText, Qt::black);
		palette.setColor(QPalette::Base, Qt::white);
		palette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
		palette.setColor(QPalette::Text, Qt::black);
		palette.setColor(QPalette::Button, QColor(240, 240, 240));
		palette.setColor(QPalette::ButtonText, Qt::black);
		palette.setColor(QPalette::Mid, QColor(160, 160, 160));
		palette.setColor(QPalette::Dark, QColor(130, 130, 130));
		palette.setColor(QPalette::Shadow, QColor(80, 80, 80));
		palette.setColor(QPalette::Highlight, QColor(70, 130, 200));
		palette.setColor(QPalette::HighlightedText, Qt::white);
	}
	else
	{
		palette.setColor(QPalette::Window, get_color(color_name_t::window_background));
		palette.setColor(QPalette::WindowText, get_color(color_name_t::window_text));
		palette.setColor(QPalette::Base, get_color(color_name_t::editor_background));
		palette.setColor(QPalette::AlternateBase, QColor(40, 40, 45));
		palette.setColor(QPalette::Text, get_color(color_name_t::editor_text));
		palette.setColor(QPalette::Button, QColor(45, 45, 50));
		palette.setColor(QPalette::ButtonText, QColor(220, 220, 220));
		palette.setColor(QPalette::Highlight, get_color(color_name_t::selection_background));
		palette.setColor(QPalette::HighlightedText, get_color(color_name_t::selection_text));
		palette.setColor(QPalette::Disabled, QPalette::Text, get_color(color_name_t::disabled_text));
		palette.setColor(QPalette::Disabled, QPalette::WindowText, get_color(color_name_t::disabled_text));
		palette.setColor(QPalette::Inactive, QPalette::Text, get_color(color_name_t::editor_text));
		palette.setColor(QPalette::Inactive, QPalette::Base, get_color(color_name_t::editor_background));
		palette.setColor(QPalette::Inactive, QPalette::Window, get_color(color_name_t::window_background));
	}

	QApplication::setPalette(palette);
}

void theme_system_t::apply_stylesheet() const
{
	if (m_active_theme == theme_t::light)
	{
		qApp->setStyleSheet("");
		return;
	}

	const auto window_bg = get_color(color_name_t::window_background);
	const auto selection_bg = get_color(color_name_t::selection_background);
	const auto selection_text = get_color(color_name_t::selection_text);

	const auto stylesheet = QString(
	                            "QScrollBar::add-page, QScrollBar::sub-page {"
	                            "  background: rgb(%1, %2, %3);"
	                            "}"
	                            "QMenu::item:selected, QMenuBar::item:selected {"
	                            "  background-color: rgb(%4, %5, %6);"
	                            "  color: rgb(%7, %8, %9);"
	                            "}")
	                            .arg(window_bg.red())
	                            .arg(window_bg.green())
	                            .arg(window_bg.blue())
	                            .arg(selection_bg.red())
	                            .arg(selection_bg.green())
	                            .arg(selection_bg.blue())
	                            .arg(selection_text.red())
	                            .arg(selection_text.green())
	                            .arg(selection_text.blue());

	qApp->setStyleSheet(stylesheet);
}
