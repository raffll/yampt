#pragma once

#include <QWidget>

class app_settings_t;

class editor_paths_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit editor_paths_view_t(QWidget * parent = nullptr);

	void load(const app_settings_t & settings);
	void apply(app_settings_t & settings) const;
};
