#include "editor_window.hpp"
#include "editor_tab.hpp"

#include <QCloseEvent>
#include <QVBoxLayout>

editor_window_t::editor_window_t(QWidget * parent)
    : QMainWindow(parent)
{
	setWindowTitle("yampt editor");
	resize(1400, 900);

	auto * central = new QWidget(this);
	auto * layout = new QVBoxLayout(central);
	layout->setContentsMargins(0, 0, 0, 0);

	editor_tab_ = new editor_tab_t(central);
	layout->addWidget(editor_tab_);

	setCentralWidget(central);
}

void editor_window_t::closeEvent(QCloseEvent * event)
{
	event->accept();
}
