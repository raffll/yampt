#include "status_filter_view.hpp"
#include "status_colors.hpp"
#include "status_display.hpp"
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

	layout_ = new QHBoxLayout(this);
	layout_->setContentsMargins(0, 0, 0, 0);
	layout_->setSpacing(2);

	for (size_t g = 0; g < status_groups.size(); ++g)
	{
		if (g > 0)
		{
			auto * sep = new QLabel(QString::fromUtf8("\xe2\x80\xa2"), this);
			sep->setStyleSheet("color: #aaa;");
			layout_->addWidget(sep);
		}

		add_status_group(status_groups[g]);
	}

	layout_->addStretch();
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

		layout_->addWidget(sb.button);
		status_buttons_.push_back(sb);
	}
}

void status_filter_view_t::update_counts(
    const std::map<status_t, size_t> & displayed_counts,
    const std::map<status_t, size_t> & total_counts)
{
	current_counts_ = total_counts;

	for (auto & sb : status_buttons_)
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
	return active_statuses_;
}

bool status_filter_view_t::has_filter() const
{
	return !active_statuses_.empty();
}

void status_filter_view_t::set_filter_state(const std::set<status_t> & statuses)
{
	active_statuses_ = statuses;
	solo_ = false;
	solo_status_ = status_t::untranslated;
	update_button_styles();
}

void status_filter_view_t::on_status_clicked(status_t status)
{
	if (solo_ && solo_status_ == status)
	{
		solo_ = false;
		solo_status_ = status_t::untranslated;
		active_statuses_ = saved_statuses_;
		update_button_styles();
		emit filters_changed();
		return;
	}

	saved_statuses_ = active_statuses_;
	solo_ = true;
	solo_status_ = status;
	active_statuses_ = { status };
	update_button_styles();
	emit filters_changed();
}

void status_filter_view_t::on_status_right_clicked(status_t status)
{
	if (active_statuses_.empty())
	{
		for (const auto & sb : status_buttons_)
		{
			if (sb.status != status)
				active_statuses_.insert(sb.status);
		}

		solo_ = false;
		solo_status_ = status_t::untranslated;
		update_button_styles();
		emit filters_changed();
		return;
	}

	solo_ = false;
	solo_status_ = status_t::untranslated;

	bool active = active_statuses_.count(status) > 0;

	if (active)
		active_statuses_.erase(status);
	else
		active_statuses_.insert(status);

	update_button_styles();
	emit filters_changed();
}

void status_filter_view_t::update_button_styles()
{
	static const QString inactive_style = "border: 1px solid #bbb; border-radius: 2px; padding: 2px 6px; "
	                                      "background: #f0f0f0; color: rgb(80,80,80);";

	static const QString disabled_style = "border: 1px solid #bbb; border-radius: 2px; padding: 2px 6px; "
	                                      "background: #f0f0f0; color: rgb(180,180,180);";

	bool no_filter = active_statuses_.empty();

	for (const auto & sb : status_buttons_)
	{
		if (!document_open_)
		{
			sb.button->setStyleSheet(disabled_style);
			sb.button->setEnabled(false);
			continue;
		}

		sb.button->setEnabled(true);

		bool active = no_filter || active_statuses_.count(sb.status) > 0;

		if (active)
		{
			const auto & color = get_status_color(sb.status);
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
	document_open_ = open;
	update_button_styles();
}
