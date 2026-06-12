#include "status_filter_bar.hpp"
#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

static QColor get_status_color_qt(const std::string & status)
{
	if (status == "untranslated")
		return QColor(166, 166, 166);
	if (status == "missing")
		return QColor(242, 140, 89);
	if (status == "duplicate")
		return QColor(242, 230, 102);
	if (status == "coords")
		return QColor(102, 204, 204);
	if (status == "fingerprint")
		return QColor(102, 191, 217);
	if (status == "heuristic")
		return QColor(153, 179, 230);
	if (status == "info")
		return QColor(115, 217, 191);
	if (status == "exact")
		return QColor(128, 230, 217);
	if (status == "wilderness")
		return QColor(77, 153, 77);
	if (status == "region")
		return QColor(153, 153, 89);
	if (status == "matched")
		return QColor(217, 217, 217);
	if (status == "error")
		return QColor(242, 102, 102);
	if (status == "identical")
		return QColor(140, 191, 140);
	if (status == "translated")
		return QColor(128, 230, 128);
	if (status == "reused")
		return QColor(128, 217, 179);
	if (status == "adapted")
		return QColor(179, 140, 217);
	if (status == "changed")
		return QColor(242, 179, 102);
	if (status == "in_progress")
		return QColor(102, 153, 242);
	return QColor(217, 217, 217);
}

static const char * get_status_display_name_qt(const std::string & status)
{
	if (status == "untranslated")
		return "Untranslated";
	if (status == "missing")
		return "Missing";
	if (status == "duplicate")
		return "Duplicate";
	if (status == "coords")
		return "Coords";
	if (status == "fingerprint")
		return "Fingerprint";
	if (status == "heuristic")
		return "Heuristic";
	if (status == "info")
		return "Info";
	if (status == "exact")
		return "Exact";
	if (status == "wilderness")
		return "Wilderness";
	if (status == "region")
		return "Region";
	if (status == "matched")
		return "Matched";
	if (status == "error")
		return "Error";
	if (status == "identical")
		return "Identical";
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
	return status.c_str();
}

status_filter_bar_t::status_filter_bar_t(QWidget * parent)
	: QWidget(parent)
{
	layout_ = new QHBoxLayout(this);
	layout_->setContentsMargins(0, 0, 0, 0);
	layout_->setSpacing(2);

	all_button_ = new QPushButton("All", this);
	connect(all_button_, &QPushButton::clicked, this, &status_filter_bar_t::on_all_clicked);
	layout_->addWidget(all_button_);

	static const std::vector<std::string> all_statuses = {
		"untranslated", "missing", "duplicate", "coords", "fingerprint",
		"heuristic", "info", "exact", "wilderness", "region",
		"matched", "error", "identical", "translated", "reused",
		"adapted", "changed", "in_progress", "mismatch"
	};

	for (const auto & status : all_statuses)
	{
		status_button_t sb;
		sb.status = status;
		sb.count = 0;
		sb.button = new QPushButton(get_status_display_name_qt(status), this);
		sb.button->setContextMenuPolicy(Qt::CustomContextMenu);
		sb.button->setVisible(false);

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

	total_label_ = new QLabel("Total: 0", this);
	layout_->addWidget(total_label_);

	update_button_styles();
}

void status_filter_bar_t::update_counts(const std::map<std::string, size_t> & counts)
{
	current_counts_ = counts;
	rebuild_buttons();
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

void status_filter_bar_t::rebuild_buttons()
{
	size_t total = 0;
	bool has_any_data = false;

	for (const auto & [status, count] : current_counts_)
	{
		if (count > 0)
			has_any_data = true;
	}

	for (auto & sb : status_buttons_)
	{
		auto it = current_counts_.find(sb.status);
		size_t count = (it != current_counts_.end()) ? it->second : 0;
		sb.count = count;
		total += count;

		if (has_any_data)
		{
			sb.button->setText(QString("%1 (%2)").arg(get_status_display_name_qt(sb.status)).arg(count));
			sb.button->setVisible(true);
		}
		else
		{
			sb.button->setVisible(false);
		}
	}

	total_label_->setText(QString("Total: %1").arg(total));
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
	active_statuses_.clear();
	active_statuses_.insert(status);
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

	if (active_statuses_.count(status))
		active_statuses_.erase(status);
	else
		active_statuses_.insert(status);

	update_button_styles();
	emit filters_changed();
}

void status_filter_bar_t::on_all_clicked()
{
	active_statuses_.clear();
	solo_ = false;
	solo_status_.clear();
	update_button_styles();
	emit filters_changed();
}

void status_filter_bar_t::update_button_styles()
{
	static const QString inactive_style =
		"background-color: transparent; color: rgb(80,80,80); border: 1px solid rgb(150,150,150); padding: 2px 6px;";

	static const QString all_active_style =
		"background-color: rgb(70,130,200); color: white; border: 1px solid rgb(50,100,170); padding: 2px 6px;";

	bool no_filter = active_statuses_.empty();
	all_button_->setStyleSheet(no_filter ? all_active_style : inactive_style);

	for (const auto & sb : status_buttons_)
	{
		bool active = no_filter || active_statuses_.count(sb.status) > 0;
		if (active)
		{
			const auto & color = get_status_color_qt(sb.status);
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
