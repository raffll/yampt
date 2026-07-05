#pragma once

#include <utility/tools.hpp>
#include <QPlainTextEdit>

class QKeyEvent;
class QMimeData;

class translation_edit_view_t : public QPlainTextEdit
{
	Q_OBJECT

public:
	explicit translation_edit_view_t(QWidget * parent = nullptr);

	void set_block_multiline(bool value);
	void set_auto_capitalize(bool value);
	void set_show_whitespace(bool value);
	void set_record_type(tools_t::rec_type_t type);

	using QPlainTextEdit::blockBoundingGeometry;
	using QPlainTextEdit::blockBoundingRect;
	using QPlainTextEdit::contentOffset;
	using QPlainTextEdit::firstVisibleBlock;

protected:
	void keyPressEvent(QKeyEvent * event) override;
	void insertFromMimeData(const QMimeData * source) override;
	void paintEvent(QPaintEvent * event) override;

signals:
	void navigate_next();
	void navigate_prev();

private:
	bool handle_navigation_keys(QKeyEvent * event);
	bool handle_multiline_guard(QKeyEvent * event);
	void apply_auto_capitalize();

	bool m_block_multiline = false;
	bool m_auto_capitalize = false;
	bool m_show_whitespace = false;
	tools_t::rec_type_t m_record_type = tools_t::rec_type_t::unknown;
};
