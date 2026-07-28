#pragma once

#include <scanner/conflict_detector.hpp>
#include <QWidget>

class QTreeView;
class lua_detail_model_t;

class lua_detail_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit lua_detail_view_t(QWidget * parent = nullptr);

	void show_conflict(const handler_conflict_t & conflict);
	void show_registration(const handler_registration_t & registration);
	void clear();

private:
	void setup_tree();
	void apply_column_sizing();
	void resizeEvent(QResizeEvent * event) override;

	QTreeView * m_tree = nullptr;
	lua_detail_model_t * m_model = nullptr;
};
