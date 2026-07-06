#pragma once

#include "../editor/glossary.hpp"
#include <utility/domain_types.hpp>
#include <vector>
#include <QWidget>

class QListWidget;
class QListWidgetItem;

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

private:
	void on_item_clicked(QListWidgetItem * item);

	QListWidget * m_list = nullptr;
};
