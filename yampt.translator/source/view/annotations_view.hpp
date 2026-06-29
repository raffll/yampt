#pragma once

#include <utility/tools.hpp>
#include "../controller/glossary.hpp"
#include <QWidget>
#include <vector>

class QListWidget;
class QListWidgetItem;
class QPushButton;

class annotations_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit annotations_view_t(QWidget * parent = nullptr);

	void update_annotations(
	    const std::vector<annotation_t> & annotations,
	    const std::string & speaker_name,
	    const std::string & gender,
	    const std::string & enchantment);
	void clear();

signals:
	void annotation_clicked(const std::string & new_text);
	void rebuild_requested();

private:
	void on_item_clicked(QListWidgetItem * item);

	QListWidget * list_ = nullptr;
	QPushButton * rebuild_btn_ = nullptr;
};
