#pragma once

#include <QWidget>

class settings_store_t;

class editor_paths_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit editor_paths_view_t(QWidget * parent = nullptr);

	void load(const settings_store_t & settings);
	void apply(settings_store_t & settings) const;
};
