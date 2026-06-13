#include "filter_bar.hpp"
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

static const char * get_type_display_name(tools_t::rec_type_t type)
{
	switch (type)
	{
	case tools_t::rec_type_t::cell:
		return "Cells";
	case tools_t::rec_type_t::dial:
		return "Topics";
	case tools_t::rec_type_t::info:
		return "Dialogues";
	case tools_t::rec_type_t::fnam:
		return "Names";
	case tools_t::rec_type_t::text:
		return "Books";
	case tools_t::rec_type_t::gmst:
		return "Settings";
	case tools_t::rec_type_t::desc:
		return "Descriptions";
	case tools_t::rec_type_t::rnam:
		return "Factions";
	case tools_t::rec_type_t::indx:
		return "Index";
	case tools_t::rec_type_t::bnam:
		return "Scripts (BNAM)";
	case tools_t::rec_type_t::sctx:
		return "Scripts (SCTX)";
	default:
		return "Unknown";
	}
}

static const std::vector<tools_t::rec_type_t> type_order = {
	tools_t::rec_type_t::cell,
	tools_t::rec_type_t::dial,
	tools_t::rec_type_t::info,
	tools_t::rec_type_t::fnam,
	tools_t::rec_type_t::text,
	tools_t::rec_type_t::gmst,
	tools_t::rec_type_t::desc,
	tools_t::rec_type_t::rnam,
	tools_t::rec_type_t::indx,
	tools_t::rec_type_t::bnam,
	tools_t::rec_type_t::sctx,
};

static const std::vector<std::string> info_sub_types = {
	"Topic", "Voice", "Greeting", "Persuasion", "Journal"
};

static const std::vector<std::string> fnam_sub_types = {
	"ACTI", "ALCH", "APPA", "ARMO", "BOOK", "BSGN", "CLAS", "CLOT",
	"CONT", "CREA", "DOOR", "FACT", "INGR", "LIGH", "LOCK", "MISC",
	"NPC_", "PROB", "RACE", "REGN", "REPA", "SPEL", "WEAP"
};

static const std::vector<std::string> desc_sub_types = {
	"Birthsigns", "Classes", "Races"
};

static const std::vector<std::string> indx_sub_types = {
	"Skills", "Magic Effects"
};

filter_bar_t::filter_bar_t(QWidget * parent)
	: QWidget(parent)
{
	auto * root_layout = new QVBoxLayout(this);
	root_layout->setContentsMargins(0, 0, 0, 0);
	root_layout->setSpacing(4);

	auto * top_row = new QWidget(this);
	auto * top_layout = new QHBoxLayout(top_row);
	top_layout->setContentsMargins(0, 0, 0, 0);
	top_layout->setSpacing(2);

	all_button_ = new QPushButton("All", top_row);
	connect(all_button_, &QPushButton::clicked, this, &filter_bar_t::on_all_clicked);
	top_layout->addWidget(all_button_);

	for (const auto & type : type_order)
	{
		type_button_t tb;
		tb.type = type;
		tb.button = new QPushButton(get_type_display_name(type), top_row);
		tb.button->setContextMenuPolicy(Qt::CustomContextMenu);

		connect(tb.button, &QPushButton::clicked, this, [this, type]() {
			on_type_clicked(type);
		});

		connect(tb.button, &QWidget::customContextMenuRequested, this, [this, type]() {
			on_type_right_clicked(type);
		});

		top_layout->addWidget(tb.button);
		type_buttons_.push_back(tb);
		active_types_.insert(type);
	}

	top_layout->addStretch();
	root_layout->addWidget(top_row);

	auto * row1 = new QWidget(this);
	auto * row1_layout = new QHBoxLayout(row1);
	row1_layout->setContentsMargins(0, 0, 0, 0);
	row1_layout->setSpacing(2);

	auto * row2 = new QWidget(this);
	sub_type_layout_ = new QHBoxLayout(row2);
	sub_type_layout_->setContentsMargins(0, 0, 0, 0);
	sub_type_layout_->setSpacing(2);

	auto add_group = [this](QHBoxLayout * layout, QWidget * parent, const std::vector<std::string> & entries)
	{
		for (const auto & entry : entries)
		{
			auto * btn = new QPushButton(QString::fromStdString(entry), parent);
			btn->setContextMenuPolicy(Qt::CustomContextMenu);

			connect(btn, &QPushButton::clicked, this, [this, entry]() {
				on_sub_type_clicked(entry);
			});

			connect(btn, &QWidget::customContextMenuRequested, this, [this, entry]() {
				on_sub_type_right_clicked(entry);
			});

			layout->addWidget(btn);
			sub_type_buttons_.push_back(btn);
			active_sub_types_.insert(entry);
		}
	};

	add_group(row1_layout, row1, fnam_sub_types);
	row1_layout->addStretch();

	add_group(sub_type_layout_, row2, info_sub_types);
	sub_type_layout_->addSpacing(8);

	add_group(sub_type_layout_, row2, desc_sub_types);
	sub_type_layout_->addSpacing(8);

	add_group(sub_type_layout_, row2, indx_sub_types);
	sub_type_layout_->addStretch();

	sub_type_bar_ = new QWidget(this);
	root_layout->addWidget(row1);
	root_layout->addWidget(row2);

	update_button_styles();
	update_sub_type_styles();
}

void filter_bar_t::update_counts(const std::map<tools_t::rec_type_t, size_t> & counts)
{
	for (auto & tb : type_buttons_)
	{
		auto it = counts.find(tb.type);
		tb.count = (it != counts.end()) ? it->second : 0;
		QString text = QString("%1 (%2)").arg(get_type_display_name(tb.type)).arg(tb.count);
		tb.button->setText(text);
	}
}

void filter_bar_t::set_total_count(size_t total)
{
	all_button_->setText(QString("All (%1)").arg(total));
}

std::set<tools_t::rec_type_t> filter_bar_t::get_active_types() const
{
	return active_types_;
}

std::set<std::string> filter_bar_t::get_active_sub_types() const
{
	return active_sub_types_;
}

bool filter_bar_t::is_solo() const
{
	return solo_;
}

tools_t::rec_type_t filter_bar_t::get_solo_type() const
{
	return solo_type_;
}

void filter_bar_t::set_filter_state(
	const std::set<tools_t::rec_type_t> & types, bool solo, tools_t::rec_type_t solo_type)
{
	active_types_ = types;
	solo_ = solo;
	solo_type_ = solo_type;
	update_button_styles();
	rebuild_sub_type_bar();
}

void filter_bar_t::on_type_clicked(tools_t::rec_type_t type)
{
	if (solo_ && solo_type_ == type)
	{
		solo_ = false;
		solo_type_ = tools_t::rec_type_t::unknown;
		active_types_ = saved_types_;
		update_button_styles();
		rebuild_sub_type_bar();
		emit filters_changed();
		return;
	}

	saved_types_ = active_types_;
	solo_ = true;
	solo_type_ = type;
	active_types_.clear();
	active_types_.insert(type);
	update_button_styles();
	rebuild_sub_type_bar();
	emit filters_changed();
}

void filter_bar_t::on_type_right_clicked(tools_t::rec_type_t type)
{
	if (solo_)
	{
		solo_ = false;
		solo_type_ = tools_t::rec_type_t::unknown;
	}

	if (active_types_.count(type))
		active_types_.erase(type);
	else
		active_types_.insert(type);

	update_button_styles();
	rebuild_sub_type_bar();
	emit filters_changed();
}

void filter_bar_t::on_all_clicked()
{
	active_types_.clear();
	for (const auto & type : type_order)
		active_types_.insert(type);

	solo_ = false;
	solo_type_ = tools_t::rec_type_t::unknown;
	update_button_styles();
	rebuild_sub_type_bar();
	emit all_reset_requested();
	emit filters_changed();
}

void filter_bar_t::update_button_styles()
{
	static const QString active_style =
		"background-color: rgb(70,130,200); color: white; border: 1px solid rgb(50,100,170); padding: 2px 6px;";
	static const QString inactive_style =
		"background-color: transparent; color: rgb(80,80,80); border: 1px solid rgb(150,150,150); padding: 2px 6px;";

	bool all_active = (active_types_.size() == type_order.size());
	all_button_->setStyleSheet(all_active ? active_style : inactive_style);

	for (const auto & tb : type_buttons_)
	{
		bool active = active_types_.count(tb.type) > 0;
		tb.button->setStyleSheet(active ? active_style : inactive_style);
	}
}

void filter_bar_t::rebuild_sub_type_bar()
{
	active_sub_types_.clear();
	sub_type_solo_ = false;
	sub_type_solo_value_.clear();
	saved_sub_types_.clear();

	for (auto * btn : sub_type_buttons_)
		active_sub_types_.insert(btn->text().toStdString());

	update_sub_type_styles();
}

void filter_bar_t::on_sub_type_clicked(const std::string & value)
{
	if (sub_type_solo_ && sub_type_solo_value_ == value)
	{
		sub_type_solo_ = false;
		sub_type_solo_value_.clear();
		active_sub_types_ = saved_sub_types_;
		update_sub_type_styles();
		emit filters_changed();
		return;
	}

	saved_sub_types_ = active_sub_types_;
	sub_type_solo_ = true;
	sub_type_solo_value_ = value;
	active_sub_types_.clear();
	active_sub_types_.insert(value);
	update_sub_type_styles();
	emit filters_changed();
}

void filter_bar_t::on_sub_type_right_clicked(const std::string & value)
{
	if (sub_type_solo_)
	{
		sub_type_solo_ = false;
		sub_type_solo_value_.clear();
	}

	if (active_sub_types_.count(value))
		active_sub_types_.erase(value);
	else
		active_sub_types_.insert(value);

	update_sub_type_styles();
	emit filters_changed();
}


void filter_bar_t::update_sub_type_styles()
{
	static const QString active_style =
		"background-color: rgb(70,130,200); color: white; border: 1px solid rgb(50,100,170); padding: 2px 6px;";
	static const QString inactive_style =
		"background-color: transparent; color: rgb(80,80,80); border: 1px solid rgb(150,150,150); padding: 2px 6px;";
	static const QString disabled_style =
		"background-color: transparent; color: rgb(180,180,180); border: 1px solid rgb(210,210,210); padding: 2px 6px;";

	static const std::set<std::string> info_set(info_sub_types.begin(), info_sub_types.end());
	static const std::set<std::string> fnam_set(fnam_sub_types.begin(), fnam_sub_types.end());
	static const std::set<std::string> desc_set(desc_sub_types.begin(), desc_sub_types.end());
	static const std::set<std::string> indx_set(indx_sub_types.begin(), indx_sub_types.end());

	bool info_active = active_types_.count(tools_t::rec_type_t::info) > 0 || active_types_.count(tools_t::rec_type_t::bnam) > 0;
	bool fnam_active = active_types_.count(tools_t::rec_type_t::fnam) > 0;
	bool desc_active = active_types_.count(tools_t::rec_type_t::desc) > 0;
	bool indx_active = active_types_.count(tools_t::rec_type_t::indx) > 0;

	for (auto * btn : sub_type_buttons_)
	{
		const auto label = btn->text().toStdString();

		bool parent_active = false;
		if (info_set.count(label))
			parent_active = info_active;
		else if (fnam_set.count(label))
			parent_active = fnam_active;
		else if (desc_set.count(label))
			parent_active = desc_active;
		else if (indx_set.count(label))
			parent_active = indx_active;

		if (!parent_active)
		{
			btn->setStyleSheet(disabled_style);
			btn->setEnabled(false);
			continue;
		}

		btn->setEnabled(true);
		bool active = active_sub_types_.count(label) > 0;
		btn->setStyleSheet(active ? active_style : inactive_style);
	}
}
