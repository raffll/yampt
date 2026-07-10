#include "validation_view.hpp"
#include <QHBoxLayout>
#include <QLabel>

validation_view_t::validation_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_label = new QLabel(tr("0 / 0"), this);
	layout->addWidget(m_label);
}

void validation_view_t::update_validation(const validation_result_t & result)
{
	if (result.limit == 0)
		m_label->setText(QString("| chars: %1").arg(result.byte_count));
	else
		m_label->setText(QString("| chars: %1 / %2").arg(result.byte_count).arg(result.limit));

	switch (result.level)
	{
	case validation_level_t::ok:
		m_label->setStyleSheet("color: rgb(50, 150, 50);");
		break;

	case validation_level_t::caution:
		m_label->setStyleSheet("color: rgb(200, 180, 0);");
		break;

	case validation_level_t::error:
		m_label->setStyleSheet("color: rgb(220, 50, 50);");
		break;
	}
}

void validation_view_t::clear()
{
	m_label->setText("");
	m_label->setStyleSheet("");
}
