#pragma once

#include "../editor/byte_limit_validator.hpp"
#include <utility/tools.hpp>
#include <QWidget>

class QLabel;

class validation_view_t : public QWidget
{
	Q_OBJECT

public:
	explicit validation_view_t(QWidget * parent = nullptr);

	void update_validation(const validation_result_t & result);
	void clear();

private:
	QLabel * label_ = nullptr;
};
