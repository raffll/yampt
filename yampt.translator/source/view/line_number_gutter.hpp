#pragma once

#include <QWidget>

class translation_edit_view_t;

class line_number_gutter_t : public QWidget
{
	Q_OBJECT

public:
	explicit line_number_gutter_t(translation_edit_view_t * editor, QWidget * parent = nullptr);

	QSize sizeHint() const override;

protected:
	void paintEvent(QPaintEvent * event) override;

private:
	int calculate_width() const;

	translation_edit_view_t * editor_ = nullptr;
};
