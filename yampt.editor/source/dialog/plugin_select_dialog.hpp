#pragma once

#include <string>
#include <vector>
#include <QDialog>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

class plugin_select_dialog_t : public QDialog
{
	Q_OBJECT

public:
	explicit plugin_select_dialog_t(const std::vector<std::string> & available_files, QWidget * parent = nullptr);

	std::vector<std::string> selected_paths() const;

private:
	QListWidget * list_ = nullptr;
	QDialogButtonBox * buttons_ = nullptr;
};
