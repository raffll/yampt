#include "view/preview_view.hpp"
#include <utility/char_diff.hpp>
#include <QHBoxLayout>
#include <QTextEdit>

preview_view_t::preview_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(4);

	m_left_edit = new QTextEdit(this);
	m_left_edit->setReadOnly(true);
	m_left_edit->setPlaceholderText("Previous plugin");

	m_right_edit = new QTextEdit(this);
	m_right_edit->setReadOnly(true);
	m_right_edit->setPlaceholderText("Selected plugin");

	layout->addWidget(m_left_edit);
	layout->addWidget(m_right_edit);
}

void preview_view_t::show_comparison(const std::string & left_text, const std::string & right_text)
{
	if (left_text.empty())
	{
		m_left_edit->clear();
		m_right_edit->setPlainText(QString::fromStdString(right_text));
		return;
	}

	if (right_text.empty())
	{
		m_left_edit->setPlainText(QString::fromStdString(left_text));
		m_right_edit->clear();
		return;
	}

	if (left_text == right_text)
	{
		m_left_edit->setPlainText(QString::fromStdString(left_text));
		m_right_edit->setPlainText(QString::fromStdString(right_text));
		return;
	}

	const auto segments = compute_char_diff(left_text, right_text);

	QString left_html;
	QString right_html;

	for (const auto & segment : segments)
	{
		auto escaped = QString::fromStdString(segment.text).toHtmlEscaped().replace("\n", "<br>");

		switch (segment.operation)
		{
		case diff_op_t::unchanged:
			left_html += escaped;
			right_html += escaped;
			break;
		case diff_op_t::deleted:
			left_html += "<span style='background-color:#ffcccc;'>" + escaped + "</span>";
			break;
		case diff_op_t::inserted:
			right_html += "<span style='background-color:#ccffcc;'>" + escaped + "</span>";
			break;
		}
	}

	m_left_edit->setHtml(left_html);
	m_right_edit->setHtml(right_html);
}

void preview_view_t::clear()
{
	m_left_edit->clear();
	m_right_edit->clear();
}
