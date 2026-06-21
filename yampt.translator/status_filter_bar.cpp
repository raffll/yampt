#include "status_filter_bar.hpp"
#include "status_colors.hpp"
#include <QHBoxLayout>
#include <QLabel>
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
	if (status == "heuristic")
		return "Heuristic";
	if (status == "error")
		return "Error";
	if (status == "translated")
		return "Translated";
	if (status == "identical")
		return "Identical";
	if (status == "reused")
		return "Reused";
	if (status == "adapted")
		return "Adapted";
	if (status == "changed")
		return "Changed";
	if (status == "in_progress")
		return "In Progress";
	if (status == "model")
		return "Model";
	if (status == "mismatch")
		return "Mismatch";
	if (status == "propagated")
		return "Propagated";
	if (status == "ambiguous")
		return "Ambiguous";
	return status.c_str();
}

static const char * get_status_tooltip(const std::string & status)
{
	if (status == "matched")
		return "Paired via fingerprint, coordinates, or position";
	if (status == "heuristic")
		return "Matched via translation engine word overlap";
	if (status == "missing")
		return "No matching record found in the other ESM";
	if (status == "duplicate")
		return "Multiple records share the same key";
	if (status == "mismatch")
		return "Record exists but original text differs";
	if (status == "translated")
		return "Translation present and unchanged";
	if (status == "identical")
		return "Original equals translation (may need work)";
	if (status == "reused")
		return "Translation copied from another entry";
	if (status == "adapted")
		return "Translation adapted from a similar entry";
	if (status == "changed")
		return "Original text changed since last translation";
	if (status == "untranslated")
		return "No translation provided yet";
	if (status == "in_progress")
		return "Translation edited but not finalized";
	if (status == "model")
		return "Translated by the translation model";
	if (status == "propagated")
		return "Translation propagated from another record";
	if (status == "ambiguous")
		return "Multiple conflicting translations found in base dicts";
	if (status == "error")
		return "Translation has a validation error";
	return "";
}

status_filter_bar_t::status_filter_bar_t(QWidget * parent)
    : QWidget(parent)
{
	setFixedHeight(26);

	layout_ = new QHBoxLayout(this);
	layout_->setContentsMargins(0, 0, 0, 0);
	layout_->setSpacing(2);

	static const std::vector<std::string> base_statuses = { "matched", "heuristic", "duplicate", "missing",
		                                                     "mismatch" };

	static const std::vector<std::string> single_statuses = { "translated", "identical", "reused",    "adapted",
		                                                      "changed",    "ambiguous",  "untranslated" };

	static const std::vector<std::string> work_statuses = { "in_progress", "propagated", "error" };

	for (const auto & status : base_statuses)
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

	auto * sep1 = new QLabel(QString::fromUtf8("\xE2\x80\xA2"), this);
	sep1->setStyleSheet("color: rgb(150,150,150);");
	layout_->addWidget(sep1);

	for (const auto & status : single_statuses)
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

	auto * sep2 = new QLabel(QString::fromUtf8("\xE2\x80\xA2"), this);
	sep2->setStyleSheet("color: rgb(150,150,150);");
	layout_->addWidget(sep2);

	for (const auto & status : work_statuses)
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

	auto * sep3 = new QLabel(QString::fromUtf8("\xE2\x80\xA2"), this);
	sep3->setStyleSheet("color: rgb(150,150,150);");
	layout_->addWidget(sep3);

	{
		status_button_t sb;
		sb.status = "model";
		sb.count = 0;
		sb.button = new QPushButton(get_status_display_name_qt("model"), this);
		sb.button->setContextMenuPolicy(Qt::CustomContextMenu);
		sb.button->setToolTip(get_status_tooltip("model"));

		connect(sb.button, &QPushButton::clicked, this, [this]() { on_status_clicked("model"); });

		connect(sb.button, &QWidget::customContextMenuRequested, this, [this]() { on_status_right_clicked("model"); });

		layout_->addWidget(sb.button);
		status_buttons_.push_back(sb);
	}

	layout_->addStretch();
	update_button_styles();
}

void status_filter_bar_t::update_counts(
    const std::map<std::string, size_t> & displayed_counts,
    const std::map<std::string, size_t> & total_counts)
{
	current_counts_ = total_counts;

	static const std::vector<std::string> matched_group = { "matched", "fingerprint", "coords",
		                                                     "exact",   "info",        "wilderness", "region" };

	static const std::vector<std::string> translated_group = { "translated" };

	for (auto & sb : status_buttons_)
	{
		size_t displayed = 0;
		size_t total = 0;

		if (sb.status == "matched")
		{
			for (const auto & s : matched_group)
			{
				auto it = displayed_counts.find(s);
				if (it != displayed_counts.end())
					displayed += it->second;

				auto it2 = total_counts.find(s);
				if (it2 != total_counts.end())
					total += it2->second;
			}
		}
		else if (sb.status == "translated")
		{
			for (const auto & s : translated_group)
			{
				auto it = displayed_counts.find(s);
				if (it != displayed_counts.end())
					displayed += it->second;

				auto it2 = total_counts.find(s);
				if (it2 != total_counts.end())
					total += it2->second;
			}
		}
		else
		{
			auto it = displayed_counts.find(sb.status);
			if (it != displayed_counts.end())
				displayed = it->second;

			auto it2 = total_counts.find(sb.status);
			if (it2 != total_counts.end())
				total = it2->second;
		}

		sb.count = total;
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

static std::set<std::string> expand_status_group(const std::string & status)
{
	if (status == "matched")
		return { "matched", "fingerprint", "coords", "exact", "info", "wilderness", "region" };

	if (status == "translated")
		return { "translated" };

	return { status };
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
	    "border: 1px solid #bbb; border-radius: 2px; padding: 2px 6px; background: #f0f0f0; color: rgb(80,80,80);";

	static const QString disabled_style =
	    "border: 1px solid #bbb; border-radius: 2px; padding: 2px 6px; background: #f0f0f0; color: rgb(180,180,180);";

	static const std::set<std::string> base_statuses = { "matched", "heuristic", "missing", "duplicate", "mismatch" };

	static const std::set<std::string> user_statuses = { "translated",   "identical",  "reused",      "adapted",
		                                                 "changed",      "ambiguous",  "untranslated", "in_progress",
		                                                 "model",        "propagated", "error" };

	bool no_filter = active_statuses_.empty();

	for (const auto & sb : status_buttons_)
	{
		bool disabled = false;
		if (dict_mode_ == dict_mode_t::none)
			disabled = true;
		else if (dict_mode_ == dict_mode_t::base && user_statuses.count(sb.status))
			disabled = true;
		else if (dict_mode_ == dict_mode_t::user && base_statuses.count(sb.status))
			disabled = true;

		if (disabled)
		{
			sb.button->setStyleSheet(disabled_style);
			sb.button->setEnabled(false);
			continue;
		}

		sb.button->setEnabled(true);

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
			sb.button->setStyleSheet(QString(
			                             "border: 1px solid rgb(%5,%6,%7); border-radius: 2px; padding: 2px 6px;"
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

void status_filter_bar_t::set_dict_mode(dict_mode_t mode)
{
	dict_mode_ = mode;
	update_button_styles();
}
