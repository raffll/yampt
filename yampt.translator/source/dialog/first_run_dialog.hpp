#pragma once

#include <string>
#include <QDialog>

class QComboBox;

class first_run_dialog_t : public QDialog
{
	Q_OBJECT

public:
	explicit first_run_dialog_t(QWidget * parent = nullptr);

	std::string selected_foreign_language() const;
	std::string selected_native_language() const;

private:
	QComboBox * from_combo_ = nullptr;
	QComboBox * to_combo_ = nullptr;
};
