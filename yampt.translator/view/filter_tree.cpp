#include "filter_tree.hpp"
#include <QListWidget>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

static const int role_counter = Qt::UserRole + 100;

class filter_delegate_t : public QStyledItemDelegate
{
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override
	{
		QStyledItemDelegate::paint(painter, option, index);

		auto counter = index.data(role_counter).toString();
		if (counter.isEmpty())
			return;

		painter->save();
		auto color = index.data(Qt::ForegroundRole).value<QBrush>().color();
		painter->setPen(color);
		auto rect = option.rect.adjusted(0, 0, -6, 0);
		painter->drawText(rect, Qt::AlignRight | Qt::AlignVCenter, counter);
		painter->restore();
	}
};

static const std::vector<std::pair<tools_t::rec_type_t, const char *>> type_order = {
	{ tools_t::rec_type_t::cell, "Cells" },        { tools_t::rec_type_t::dial, "Topics" },
	{ tools_t::rec_type_t::info, "Dialogues" },    { tools_t::rec_type_t::fnam, "Names" },
	{ tools_t::rec_type_t::text, "Books" },        { tools_t::rec_type_t::gmst, "Settings" },
	{ tools_t::rec_type_t::desc, "Descriptions" }, { tools_t::rec_type_t::rnam, "Factions" },
	{ tools_t::rec_type_t::indx, "Index" },        { tools_t::rec_type_t::sctx, "Scripts" },
};

static const std::vector<std::pair<std::string, const char *>> info_sub_types = {
	{ "Topic", "Dialogues: Topic" },       { "Voice", "Dialogues: Voice" },
	{ "Greeting", "Dialogues: Greeting" }, { "Persuasion", "Dialogues: Persuasion" },
	{ "Journal", "Dialogues: Journal" },
};

static const std::vector<std::pair<std::string, const char *>> fnam_sub_types = {
	{ "ACTI", "Names: Activators" },    { "ALCH", "Names: Potions" },  { "APPA", "Names: Apparatus" },
	{ "ARMO", "Names: Armor" },         { "BOOK", "Names: Books" },    { "BSGN", "Names: Birthsigns" },
	{ "CLAS", "Names: Classes" },       { "CLOT", "Names: Clothing" }, { "CONT", "Names: Containers" },
	{ "CREA", "Names: Creatures" },     { "DOOR", "Names: Doors" },    { "FACT", "Names: Factions" },
	{ "INGR", "Names: Ingredients" },   { "LIGH", "Names: Lights" },   { "LOCK", "Names: Lockpicks" },
	{ "MISC", "Names: Miscellaneous" }, { "NPC_", "Names: NPCs" },     { "PROB", "Names: Probes" },
	{ "RACE", "Names: Races" },         { "REGN", "Names: Regions" },  { "REPA", "Names: Repair Items" },
	{ "SPEL", "Names: Spells" },        { "WEAP", "Names: Weapons" },
};

static const std::vector<std::pair<std::string, const char *>> desc_sub_types = {
	{ "Birthsigns", "Descriptions: Birthsigns" },
	{ "Classes", "Descriptions: Classes" },
	{ "Races", "Descriptions: Races" },
};

static const std::vector<std::pair<std::string, const char *>> indx_sub_types = {
	{ "Skills", "Index: Skills" },
	{ "Magic Effects", "Index: Magic Effects" },
};

static const QColor color_selected_bg(90, 155, 230);
static const QColor color_selected_fg(255, 255, 255);
static const QColor color_deselected_fg(80, 80, 80);
static const QColor color_disabled_fg(180, 180, 180);

filter_tree_t::filter_tree_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	list_ = new QListWidget(this);
	list_->setSelectionMode(QAbstractItemView::NoSelection);
	list_->setFocusPolicy(Qt::NoFocus);
	list_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	list_->setItemDelegate(new filter_delegate_t(list_));
	layout->addWidget(list_);

	build_rows();
	update_styles();

	connect(list_, &QListWidget::itemClicked, this, &filter_tree_t::on_item_clicked);
	list_->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(
	    list_,
	    &QWidget::customContextMenuRequested,
	    this,
	    [this](const QPoint & pos)
	{
		auto * item = list_->itemAt(pos);
		if (item)
			on_item_right_clicked(item);
	});
}

void filter_tree_t::build_rows()
{
	auto add_row =
	    [this](item_kind_t kind, tools_t::rec_type_t type, const std::string & sub_type, const char * label) -> int
	{
		auto * item = new QListWidgetItem(label, list_);
		item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		int idx = static_cast<int>(rows_.size());
		rows_.push_back({ item, kind, type, sub_type, true });
		return idx;
	};

	all_row_ = add_row(item_kind_t::all, tools_t::rec_type_t::unknown, {}, "All");

	auto add_sub_types_for =
	    [&](tools_t::rec_type_t type, const std::vector<std::pair<std::string, const char *>> & subs)
	{
		for (const auto & [sub_id, label] : subs)
			add_row(item_kind_t::sub_type, type, sub_id, label);
	};

	for (const auto & [type, name] : type_order)
	{
		bool has_subs =
		    (type == tools_t::rec_type_t::info || type == tools_t::rec_type_t::fnam ||
		     type == tools_t::rec_type_t::desc || type == tools_t::rec_type_t::indx);

		if (has_subs)
		{
			if (type == tools_t::rec_type_t::info)
				add_sub_types_for(type, info_sub_types);
			else if (type == tools_t::rec_type_t::fnam)
				add_sub_types_for(type, fnam_sub_types);
			else if (type == tools_t::rec_type_t::desc)
				add_sub_types_for(type, desc_sub_types);
			else if (type == tools_t::rec_type_t::indx)
				add_sub_types_for(type, indx_sub_types);
		}
		else
		{
			add_row(item_kind_t::type, type, {}, name);
		}
	}

	yaml_row_ = add_row(item_kind_t::yaml, tools_t::rec_type_t::unknown, {}, "YAML");
	rows_[yaml_row_].selected = false;
	rows_[yaml_row_].item->setHidden(true);
}

void filter_tree_t::on_item_clicked(QListWidgetItem * item)
{
	if (!enabled_)
		return;

	int idx = list_->row(item);
	if (idx < 0 || idx >= static_cast<int>(rows_.size()))
		return;

	auto & row = rows_[idx];

	if (row.kind == item_kind_t::all)
	{
		for (auto & r : rows_)
		{
			if (r.kind == item_kind_t::all || r.kind == item_kind_t::yaml)
				continue;

			r.selected = true;
		}

		update_all_state();
		update_styles();
		emit all_reset_requested();
		emit filters_changed();
		return;
	}

	if (row.kind == item_kind_t::yaml)
	{
		row.selected = !row.selected;
		update_styles();
		emit filters_changed();
		return;
	}

	for (auto & r : rows_)
	{
		if (r.kind == item_kind_t::all || r.kind == item_kind_t::yaml)
			continue;

		r.selected = false;
	}

	row.selected = true;
	update_all_state();
	update_styles();
	emit filters_changed();
}

void filter_tree_t::on_item_right_clicked(QListWidgetItem * item)
{
	if (!enabled_)
		return;

	int idx = list_->row(item);
	if (idx < 0 || idx >= static_cast<int>(rows_.size()))
		return;

	auto & row = rows_[idx];

	if (row.kind == item_kind_t::all)
	{
		bool any_deselected = false;
		for (const auto & r : rows_)
		{
			if (r.kind == item_kind_t::all || r.kind == item_kind_t::yaml)
				continue;

			if (!r.selected)
			{
				any_deselected = true;
				break;
			}
		}

		bool new_state = any_deselected;
		for (auto & r : rows_)
		{
			if (r.kind == item_kind_t::all || r.kind == item_kind_t::yaml)
				continue;

			r.selected = new_state;
		}

		update_all_state();
		update_styles();
		emit all_reset_requested();
		emit filters_changed();
		return;
	}

	if (row.kind == item_kind_t::yaml)
		return;

	row.selected = !row.selected;
	update_all_state();
	update_styles();
	emit filters_changed();
}

void filter_tree_t::update_all_state()
{
	if (all_row_ < 0)
		return;

	bool all_selected = true;
	for (const auto & r : rows_)
	{
		if (r.kind == item_kind_t::all || r.kind == item_kind_t::yaml)
			continue;

		if (!r.selected)
		{
			all_selected = false;
			break;
		}
	}

	rows_[all_row_].selected = all_selected;
}

void filter_tree_t::update_styles()
{
	for (auto & r : rows_)
	{
		if (!enabled_)
		{
			r.item->setForeground(QBrush(color_disabled_fg));
			r.item->setBackground(QBrush());
			continue;
		}

		if (r.selected)
		{
			r.item->setBackground(QBrush(color_selected_bg));
			r.item->setForeground(QBrush(color_selected_fg));
		}
		else
		{
			r.item->setBackground(QBrush());
			r.item->setForeground(QBrush(color_deselected_fg));
		}
	}
}

void filter_tree_t::update_counts(
    const std::map<tools_t::rec_type_t, size_t> & total_counts,
    const std::map<tools_t::rec_type_t, size_t> & translated_counts)
{
	for (auto & r : rows_)
	{
		if (r.kind != item_kind_t::type)
			continue;

		auto total_it = total_counts.find(r.type);
		size_t total = (total_it != total_counts.end()) ? total_it->second : 0;

		auto trans_it = translated_counts.find(r.type);
		size_t translated = (trans_it != translated_counts.end()) ? trans_it->second : 0;

		r.item->setHidden(total == 0);
		r.item->setData(role_counter, QString("%1/%2").arg(translated).arg(total));
	}
}

void filter_tree_t::update_sub_type_counts(
    const std::map<std::string, size_t> & sub_type_total_counts,
    const std::map<std::string, size_t> & sub_type_translated_counts)
{
	for (auto & r : rows_)
	{
		if (r.kind != item_kind_t::sub_type)
			continue;

		auto total_it = sub_type_total_counts.find(r.sub_type);
		size_t total = (total_it != sub_type_total_counts.end()) ? total_it->second : 0;

		auto trans_it = sub_type_translated_counts.find(r.sub_type);
		size_t translated = (trans_it != sub_type_translated_counts.end()) ? trans_it->second : 0;

		r.item->setHidden(total == 0);
		r.item->setData(role_counter, QString("%1/%2").arg(translated).arg(total));
	}
}

void filter_tree_t::set_total_count(size_t translated, size_t total)
{
	if (all_row_ < 0)
		return;

	rows_[all_row_].item->setData(role_counter, QString("%1/%2").arg(translated).arg(total));
}

std::set<tools_t::rec_type_t> filter_tree_t::get_active_types() const
{
	std::set<tools_t::rec_type_t> result;

	for (const auto & r : rows_)
	{
		if (r.kind == item_kind_t::type && r.selected)
			result.insert(r.type);

		if (r.kind == item_kind_t::sub_type && r.selected)
			result.insert(r.type);
	}

	return result;
}

std::set<std::string> filter_tree_t::get_active_sub_types() const
{
	std::set<std::string> result;

	for (const auto & r : rows_)
	{
		if (r.kind == item_kind_t::sub_type && r.selected)
			result.insert(r.sub_type);
	}

	return result;
}

bool filter_tree_t::has_sub_type_filter() const
{
	for (const auto & r : rows_)
	{
		if (r.kind != item_kind_t::sub_type)
			continue;

		if (r.item->isHidden())
			continue;

		if (!r.selected)
			return true;
	}

	return false;
}

void filter_tree_t::set_active_types(const std::set<tools_t::rec_type_t> & types)
{
	for (auto & r : rows_)
	{
		if (r.kind == item_kind_t::all || r.kind == item_kind_t::yaml)
			continue;

		r.selected = types.count(r.type) > 0;
	}

	update_all_state();
	update_styles();
}

void filter_tree_t::set_active_sub_types(const std::set<std::string> & sub_types)
{
	for (auto & r : rows_)
	{
		if (r.kind != item_kind_t::sub_type)
			continue;

		r.selected = sub_types.empty() || sub_types.count(r.sub_type) > 0;
	}

	update_all_state();
	update_styles();
}

bool filter_tree_t::is_solo() const
{
	int active_count = 0;
	for (const auto & r : rows_)
	{
		if (r.kind == item_kind_t::all || r.kind == item_kind_t::yaml)
			continue;

		if (r.selected)
			++active_count;
	}

	return active_count == 1;
}

tools_t::rec_type_t filter_tree_t::get_solo_type() const
{
	for (const auto & r : rows_)
	{
		if (r.kind == item_kind_t::all || r.kind == item_kind_t::yaml)
			continue;

		if (r.selected)
			return r.type;
	}

	return tools_t::rec_type_t::unknown;
}

bool filter_tree_t::is_yaml_filter_active() const
{
	if (yaml_row_ < 0)
		return false;

	return rows_[yaml_row_].selected;
}

void filter_tree_t::set_yaml_filter_active(bool active)
{
	if (yaml_row_ < 0)
		return;

	rows_[yaml_row_].selected = active;
	update_styles();
}

void filter_tree_t::set_yaml_button_visible(bool visible)
{
	if (yaml_row_ < 0)
		return;

	rows_[yaml_row_].item->setHidden(!visible);
}

void filter_tree_t::setEnabled(bool enabled)
{
	enabled_ = enabled;
	QWidget::setEnabled(enabled);
	update_styles();
}

void filter_tree_t::set_display_mode(display_mode_t mode)
{
	switch (mode)
	{
	case display_mode_t::empty:
		for (auto & r : rows_)
			r.item->setHidden(true);
		break;

	case display_mode_t::all_only:
		for (auto & r : rows_)
			r.item->setHidden(r.kind != item_kind_t::all);
		break;

	case display_mode_t::full:
		for (auto & r : rows_)
		{
			if (r.kind == item_kind_t::yaml)
				continue;

			r.item->setHidden(false);
		}
		break;
	}
}
