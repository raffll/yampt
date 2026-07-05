#include "status_filter_view.hpp"
#include "status_display.hpp"
#include <theme_system.hpp>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

static const char * get_status_tooltip(status_t status)
{
	switch (status)
	{
	case status_t::missing:
		return "No matching record found in the other ESM";
	case status_t::duplicate:
		return "Multiple records share the same key";
	case status_t::mismatch:
		return "Record exists but original text differs";
	case status_t::translated:
		return "Translation present and unchanged";
	case status_t::reused:
		return "Translation copied from another entry";
	case status_t::adapted:
		return "Translation adapted from a similar entry";
	case status_t::changed:
		return "Original text changed since last translation";
	case status_t::outdated:
		return "Source text changed while translation was in progress";
	case status_t::untranslated:
		return "No translation provided yet";
	case status_t::in_progress:
		return "Translation edited but not finalized";
	case status_t::model:
		return "Translated by the translation model";
	case status_t::propagated:
		return "Translation propagated from another record";
	case status_t::ambiguous:
		return "Multiple conflicting translations found in base dicts";
	case status_t::error:
		return "Translation has a validation error";
	case status_t::heuristic:
		return "Matched by heuristic (needs verification)";
	case status_t::to_verify:
		return "Identical text, needs verification";
	}
	return "";
}

static const std::vector<std::vector<status_t>> status_groups = {
	{ status_t::translated, status_t::to_verify, status_t::untranslated },
	{ status_t::reused, status_t::adapted, status_t::ambiguous, status_t::changed, status_t::outdated },
	{ status_t::duplicate, status_t::heuristic, status_t::missing, status_t::mismatch },
	{ status_t::in_progress, status_t::propagated, status_t::model, status_t::error },
};

status_filter_view_t::status_filter_view_t(QWidget * parent)
    : QWidget(parent)
{
	setFixedHeight(26);

	m_layout = new QHBoxLayout(this);
	m_layout->setContentsMargins(0, 0, 0, 0);
	m_layout->setSpacing(2);

	for (size_t g = 0; g < status_groups.size(); ++g)
	{
		if (g > 0)
		{
			auto * sep = new QLabel(QString::fromUtf8("\xe2\x80\xa2"), this);
			sep->setStyleSheet("color: #aaa;");
			m_layout->addWidget(sep);
		}

		add_status_group(status_groups[g]);
	}

	m_layout->addStretch();
	update_button_styles();
}

void status_filter_view_t::add_status_group(const std::vector<status_t> & statuses)
{
	for (const auto & status : statuses)
	{
		status_button_t sb;
		sb.status = status;
		sb.count = 0;
		sb.button = new QPushButton(status_display_name(status), this);
		sb.button->setContextMenuPolicy(Qt::CustomContextMenu);
		sb.button->setToolTip(get_status_tooltip(status));

		connect(sb.button, &QPushButton::clicked, this, [this, s = status]() { on_status_clicked(s); });

		connect(
		    sb.button,
		    &QWidget::customContextMenuRequested,
		    this,
		    [this, s = status]() { on_status_right_clicked(s); });

		m_layout->addWidget(sb.button);
		m_status_buttons.push_back(sb);
	}
}

void status_filter_view_t::update_counts(
    const std::map<status_t, size_t> & displayed_counts,
    const std::map<status_t, size_t> & total_counts)
{
	m_current_counts = total_counts;

	for (auto & sb : m_status_buttons)
	{
		size_t total = 0;

		auto it2 = total_counts.find(sb.status);
		if (it2 != total_counts.end())
			total = it2->second;

		sb.count = total;

		if (total > 0)
			sb.button->setText(QString("%1 (%2)").arg(status_display_name(sb.status)).arg(total));
		else
			sb.button->setText(status_display_name(sb.status));
	}
}

std::set<status_t> status_filter_view_t::get_active_statuses() const
{
	return m_active_statuses;
}

bool status_filter_view_t::has_filter() const
{
	return !m_active_statuses.empty();
}

void status_filter_view_t::set_filter_state(const std::set<status_t> & statuses)
{
	m_active_statuses = statuses;
	m_solo = false;
	m_solo_status = status_t::untranslated;
	update_button_styles();
}

void status_filter_view_t::on_status_clicked(status_t status)
{
	if (m_solo && m_solo_status == status)
	{
		m_solo = false;
		m_solo_status = status_t::untranslated;
		m_active_statuses = m_saved_statuses;
		update_button_styles();
		emit filters_changed();
		return;
	}

	m_saved_statuses = m_active_statuses;
	m_solo = true;
	m_solo_status = status;
	m_active_statuses = { status };
	update_button_styles();
	emit filters_changed();
}

void status_filter_view_t::on_status_right_clicked(status_t status)
{
	if (m_active_statuses.empty())
	{
		for (const auto & sb : m_status_buttons)
		{
			if (sb.status != status)
				m_active_statuses.insert(sb.status);
		}

		m_solo = false;
		m_solo_status = status_t::untranslated;
		update_button_styles();
		emit filters_changed();
		return;
	}

	m_solo = false;
	m_solo_status = status_t::untranslated;

	bool active = m_active_statuses.count(status) > 0;

	if (active)
		m_active_statuses.erase(status);
	else
		m_active_statuses.insert(status);

	update_button_styles();
	emit filters_changed();
}

void status_filter_view_t::update_button_styles()
{
	const bool is_dark = theme_system_t::instance().active_theme() == theme_t::dark;

	const auto inactive_style = is_dark ? QString(
	                                          "border: 1px solid #555; border-radius: 2px; padding: 2px 6px; "
	                                          "background: #2a2a2a; color: rgb(160,160,160);")
	                                    : QString(
	                                          "border: 1px solid #bbb; border-radius: 2px; padding: 2px 6px; "
	                                          "background: #f0f0f0; color: rgb(80,80,80);");

	const auto disabled_style = is_dark ? QString(
	                                          "border: 1px solid #444; border-radius: 2px; padding: 2px 6px; "
	                                          "background: #222; color: rgb(80,80,80);")
	                                    : QString(
	                                          "border: 1px solid #bbb; border-radius: 2px; padding: 2px 6px; "
	                                          "background: #f0f0f0; color: rgb(180,180,180);");

	bool no_filter = m_active_statuses.empty();

	for (const auto & sb : m_status_buttons)
	{
		if (!m_document_open)
		{
			sb.button->setStyleSheet(disabled_style);
			sb.button->setEnabled(false);
			continue;
		}

		sb.button->setEnabled(true);

		bool active = no_filter || m_active_statuses.count(sb.status) > 0;

		if (active)
		{
			const auto & color = theme_system_t::instance().get_status_color(sb.status);
			int text_brightness = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
			QString text_color = (text_brightness > 150) ? "black" : "white";
			sb.button->setStyleSheet(QString(
			                             "border: 1px solid rgb(%5,%6,%7); border-radius: 2px; "
			                             "padding: 2px 6px;"
			                             " background-color: rgb(%1,%2,%3); color: %4;")
			                             .arg(color.red())
			                             .arg(color.green())
			                             .arg(color.blue())
			                             .arg(text_color)
			                             .arg(qMax(0, color.red() - 40))
			                             .arg(qMax(0, color.green() - 40))
			                             .arg(qMax(0, color.blue() - 40)));
		}
		else
		{
			sb.button->setStyleSheet(inactive_style);
		}
	}
}

void status_filter_view_t::set_document_open(bool open)
{
	m_document_open = open;
	update_button_styles();
}

void status_filter_view_t::refresh_theme()
{
	update_button_styles();
}
