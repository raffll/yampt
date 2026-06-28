#include "status_filter_bar.hpp"
#include "../utility/status_colors.hpp"
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

static const char * get_status_display_name_qt(const std::string & status)
{
	if (status == tools_t::status_t::untranslated)
		return "Untranslated";
	if (status == tools_t::status_t::missing)
		return "Missing";
	if (status == tools_t::status_t::duplicate)
		return "Duplicate";
	if (status == tools_t::status_t::error)
		return "Error";
	if (status == tools_t::status_t::translated)
		return "Translated";
	if (status == tools_t::status_t::reused)
		return "Reused";
	if (status == tools_t::status_t::adapted)
		return "Adapted";
	if (status == tools_t::status_t::changed)
		return "Changed";
	if (status == tools_t::status_t::outdated)
		return "Outdated";
	if (status == tools_t::status_t::in_progress)
		return "In Progress";
	if (status == tools_t::status_t::model)
		return "Model";
	if (status == tools_t::status_t::mismatch)
		return "Mismatch";
	if (status == tools_t::status_t::propagated)
		return "Propagated";
	if (status == tools_t::status_t::ambiguous)
		return "Ambiguous";
	if (status == tools_t::status_t::heuristic)
		return "Heuristic";
	if (status == tools_t::status_t::to_verify)
		return "To Verify";
	return status.c_str();
}

static const char * get_status_tooltip(const std::string & status)
{
	if (status == tools_t::status_t::missing)
		return "No matching record found in the other ESM";
	if (status == tools_t::status_t::duplicate)
		return "Multiple records share the same key";
	if (status == tools_t::status_t::mismatch)
		return "Record exists but original text differs";
	if (status == tools_t::status_t::translated)
		return "Translation present and unchanged";
	if (status == tools_t::status_t::reused)
		return "Translation copied from another entry";
	if (status == tools_t::status_t::adapted)
		return "Translation adapted from a similar entry";
	if (status == tools_t::status_t::changed)
		return "Original text changed since last translation";
	if (status == tools_t::status_t::outdated)
		return "Source text changed while translation was in progress";
	if (status == tools_t::status_t::untranslated)
		return "No translation provided yet";
	if (status == tools_t::status_t::in_progress)
		return "Translation edited but not finalized";
	if (status == tools_t::status_t::model)
		return "Translated by the translation model";
	if (status == tools_t::status_t::propagated)
		return "Translation propagated from another record";
	if (status == tools_t::status_t::ambiguous)
		return "Multiple conflicting translations found in base dicts";
	if (status == tools_t::status_t::error)
		return "Translation has a validation error";
	if (status == tools_t::status_t::heuristic)
		return "Matched by heuristic (needs verification)";
	if (status == tools_t::status_t::to_verify)
		return "Identical text, needs verification";
	return "";
}

static const std::vector<std::vector<std::string>> status_groups = {
	{ tools_t::status_t::translated, tools_t::status_t::to_verify, tools_t::status_t::untranslated },
	{ tools_t::status_t::reused,
	  tools_t::status_t::adapted,
	  tools_t::status_t::ambiguous,
	  tools_t::status_t::changed,
	  tools_t::status_t::outdated },
	{ tools_t::status_t::duplicate,
	  tools_t::status_t::heuristic,
	  tools_t::status_t::missing,
	  tools_t::status_t::mismatch },
	{ tools_t::status_t::in_progress,
	  tools_t::status_t::propagated,
	  tools_t::status_t::model,
	  tools_t::status_t::error },
};

status_filter_bar_t::status_filter_bar_t(QWidget * parent)
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

void status_filter_bar_t::add_status_group(const std::vector<std::string> & statuses)
{
	for (const auto & status : statuses)
	{
		status_button_t sb;
		sb.status = status;
		sb.count = 0;
		sb.button = new QPushButton(get_status_display_name_qt(status), this);
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

void status_filter_bar_t::update_counts(
    const std::map<std::string, size_t> & displayed_counts,
    const std::map<std::string, size_t> & total_counts)
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
			sb.button->setText(QString("%1 (%2)").arg(get_status_display_name_qt(sb.status)).arg(total));
		else
			sb.button->setText(get_status_display_name_qt(sb.status));
	}
}

std::set<std::string> status_filter_bar_t::get_active_statuses() const
{
	return active_statuses_;
}

bool status_filter_bar_t::has_filter() const
{
	return !active_statuses_.empty();
}

void status_filter_bar_t::set_filter_state(const std::set<std::string> & statuses)
{
	active_statuses_ = statuses;
	solo_ = false;
	solo_status_.clear();
	update_button_styles();
}

void status_filter_bar_t::on_status_clicked(const std::string & status)
{
	if (solo_ && solo_status_ == status)
	{
		solo_ = false;
		solo_status_.clear();
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

void status_filter_bar_t::on_status_right_clicked(const std::string & status)
{
	if (active_statuses_.empty())
	{
		for (const auto & sb : status_buttons_)
		{
			if (sb.status != status)
				active_statuses_.insert(sb.status);
		}

		solo_ = false;
		solo_status_.clear();
		update_button_styles();
		emit filters_changed();
		return;
	}

	solo_ = false;
	solo_status_.clear();

	bool active = active_statuses_.count(status) > 0;

	if (active)
		active_statuses_.erase(status);
	else
		active_statuses_.insert(status);

	update_button_styles();
	emit filters_changed();
}

void status_filter_bar_t::update_button_styles()
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

void status_filter_bar_t::set_document_open(bool open)
{
	document_open_ = open;
	update_button_styles();
}
