#pragma once

#include <utility/theme_enums.hpp>
#include <utility/color_palette.hpp>
#include <scanner/conflict_enums.hpp>
#include <utility/status_types.hpp>
#include <QColor>
#include <QObject>

class theme_system_t : public QObject
{
	Q_OBJECT

public:
	static theme_system_t & instance();

	theme_t active_theme() const;
	void set_theme(theme_t theme);

	QColor get_color(color_name_t name) const;
	QColor get_color(color_name_t name, theme_t theme) const;
	QColor get_status_color(status_t status) const;
	QColor get_status_color(status_t status, theme_t theme) const;

	QColor conflict_all_background(conflict_all_t value) const;
	QColor conflict_this_foreground(conflict_this_t value) const;

	void apply_to_application();

signals:
	void theme_changed(theme_t new_theme);

private:
	theme_system_t() = default;
	theme_t m_active_theme = theme_t::light;

	void apply_palette() const;
	void apply_stylesheet() const;
};
