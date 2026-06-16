#pragma once

#include <QWidget>

class editor_text_edit_t;

class line_number_gutter_t : public QWidget
{
	Q_OBJECT

public:
	explicit line_number_gutter_t(editor_text_edit_t * editor, QWidget * parent = nullptr);

	QSize sizeHint() const override;

protected:
	void paintEvent(QPaintEvent * event) override;

private:
	int calculate_width() const;

	editor_text_edit_t * editor_ = nullptr;
};
