#pragma once

#include "../yampt/tools.hpp"
#include "validation_manager.hpp"
#include <QWidget>

class QLabel;

class validation_indicator_t : public QWidget
{
	Q_OBJECT

public:
	explicit validation_indicator_t(QWidget * parent = nullptr);

	void update_validation(const validation_result_t & result);
	void clear();

private:
	QLabel * label_ = nullptr;
};
