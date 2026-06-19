#pragma once

#include <QMainWindow>

class editor_tab_t;

class editor_window_t : public QMainWindow
{
	Q_OBJECT

public:
	explicit editor_window_t(QWidget * parent = nullptr);

protected:
	void closeEvent(QCloseEvent * event) override;

private:
	editor_tab_t * editor_tab_ = nullptr;
};
