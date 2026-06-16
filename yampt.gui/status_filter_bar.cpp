#include "status_filter_bar.hpp"
#include "status_colors.hpp"
#include <QHBoxLayout>
#include <QPushButton>

static const char * get_status_display_name_qt(const std::string & status)
{
	if (status == "untranslated")
		return "Untranslated";
	if (status == "missing")
		return "Missing";
	if (status == "duplicate")
		return "Duplicate";
	if (status == "matched")
		return "Matched";
	if (status == "error")
		return "Error";
	if (status == "translated")
		return "Translated";
	if (status == "reused")
		return "Reused";
	if (status == "adapted")
		return "Adapted";
	if (status == "changed")
		return "Changed";
	if (status == "in_progress")
		return "In Progress";
	if (status == "mismatch")
		return "Mismatch";
	if (status == "propagated")
		return "Propagated";
	return status.c_str();
}

status_filter_bar_t::status_filter_bar_t(QWidget * parent)
	: QWidget(parent)
{
	setFixedHeight(26);

	layout_ = new QHBoxLayout(this);
	layout_->setContentsMargins(0, 0, 0, 0);
	layout_->setSpacing(2);

	static const std::vector<std::string> base_statuses = {
		"matched", "duplicate", "missing", "mismatch"
	};

	static const std::vector<std::string> single_statuses = {
		"translated", "reused", "adapted", "changed", "untranslated"
	};

	static const std::vector<std::string> work_statuses = {
		"in_progress", "propagated", "error"
	};

	for (const auto & status : base_statuses)
	{
		status_button_t sb;
		sb.status = status;
		sb.count = 0;
		sb.button = new QPushButton(get_status_display_name_qt(status), this);
		sb.button->setContextMenuPolicy(Qt::CustomContextMenu);

		connect(sb.button, &QPushButton::clicked, this, [this, s = status]() {
			on_status_clicked(s);
		});

		connect(sb.button, &QWidget::customContextMenuRequested, this, [this, s = status]() {
			on_status_right_clicked(s);
		});

		layout_->addWidget(sb.button);
		status_buttons_.push_back(sb);
	}

	layout_->addSpacing(12);

	for (const auto & status : single_statuses)
	{
		status_button_t sb;
		sb.status = status;
		sb.count = 0;
		sb.button = new QPushButton(get_status_display_name_qt(status), this);
		sb.button->setContextMenuPolicy(Qt::CustomContextMenu);

		connect(sb.button, &QPushButton::clicked, this, [this, s = status]() {
			on_status_clicked(s);
		});

		connect(sb.button, &QWidget::customContextMenuRequested, this, [this, s = status]() {
			on_status_right_clicked(s);
		});

		layout_->addWidget(sb.button);
		status_buttons_.push_back(sb);
	}

	layout_->addSpacing(12);

	for (const auto & status : work_statuses)
	{
		status_button_t sb;
		sb.status = status;
		sb.count = 0;
		sb.button = new QPushButton(get_status_display_name_qt(status), this);
		sb.button->setContextMenuPolicy(Qt::CustomContextMenu);

		connect(sb.button, &QPushButton::clicked, this, [this, s = status]() {
			on_status_clicked(s);
		});

		connect(sb.button, &QWidget::customContextMenuRequested, this, [this, s = status]() {
			on_status_right_clicked(s);
		});

		layout_->addWidget(sb.button);
		status_buttons_.push_back(sb);
	}

	layout_->addStretch();
	update_button_styles();
}

void status_filter_bar_t::update_counts(const std::map<std::string, size_t> & counts)
{
	current_counts_ = counts;

	static const std::vector<std::string> matched_group = {
		"matched", "fingerprint", "coords", "heuristic", "exact",
		"info", "wilderness", "region"
	};

	static const std::vector<std::string> translated_group = {
		"translated"
	};

	for (auto & sb : status_buttons_)
	{
		size_t total = 0;

		if (sb.status == "matched")
		{
			for (const auto & s : matched_group)
			{
				auto it = current_counts_.find(s);
				if (it != current_counts_.end())
					total += it->second;
			}
		}
		else if (sb.status == "translated")
		{
			for (const auto & s : translated_group)
			{
				auto it = current_counts_.find(s);
				if (it != current_counts_.end())
					total += it->second;
			}
		}
		else
		{
			auto it = current_counts_.find(sb.status);
			if (it != current_counts_.end())
				total = it->second;
		}

		sb.count = total;
		sb.button->setText(QString("%1 (%2)").arg(get_status_display_name_qt(sb.status)).arg(total));
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

static std::set<std::string> expand_status_group(const std::string & status)
{
	if (status == "matched")
		return {"matched", "fingerprint", "coords", "heuristic", "exact", "info", "wilderness", "region"};

	if (status == "translated")
		return {"translated"};

	return {status};
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
	active_statuses_ = expand_status_group(status);
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
			{
				auto expanded = expand_status_group(sb.status);
				active_statuses_.insert(expanded.begin(), expanded.end());
			}
		}

		solo_ = false;
		solo_status_.clear();
		update_button_styles();
		emit filters_changed();
		return;
	}

	solo_ = false;
	solo_status_.clear();

	auto expanded = expand_status_group(status);
	bool any_active = false;
	for (const auto & s : expanded)
	{
		if (active_statuses_.count(s))
		{
			any_active = true;
			break;
		}
	}

	if (any_active)
	{
		for (const auto & s : expanded)
			active_statuses_.erase(s);
	}
	else
	{
		active_statuses_.insert(expanded.begin(), expanded.end());
	}

	update_button_styles();
	emit filters_changed();
}

void status_filter_bar_t::update_button_styles()
{
	static const QString inactive_style =
		"background-color: transparent; color: rgb(80,80,80); border: 1px solid rgb(150,150,150); padding: 2px 6px;";

	bool no_filter = active_statuses_.empty();

	for (const auto & sb : status_buttons_)
	{
		auto expanded = expand_status_group(sb.status);
		bool active = no_filter;
		if (!active)
		{
			for (const auto & s : expanded)
			{
				if (active_statuses_.count(s))
				{
					active = true;
					break;
				}
			}
		}

		if (active)
		{
			const auto & color = get_status_color(sb.status);
			int text_brightness = (color.red() * 299 + color.green() * 587 + color.blue() * 114) / 1000;
			QString text_color = (text_brightness > 150) ? "black" : "white";
			sb.button->setStyleSheet(
				QString("background-color: rgb(%1,%2,%3); color: %4; border: 1px solid rgb(%5,%6,%7); padding: 2px 6px;")
					.arg(color.red())
					.arg(color.green())
					.arg(color.blue())
					.arg(text_color)
					.arg(qMax(0, color.red() - 30))
					.arg(qMax(0, color.green() - 30))
					.arg(qMax(0, color.blue() - 30)));
		}
		else
		{
			sb.button->setStyleSheet(inactive_style);
		}
	}
}
