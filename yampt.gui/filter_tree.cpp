#include "filter_tree.hpp"
#include <QHeaderView>
#include <QTreeWidget>
#include <QVBoxLayout>

static const std::vector<std::pair<tools_t::rec_type_t, const char *>> type_order = {
	{tools_t::rec_type_t::cell, "Cells"},
	{tools_t::rec_type_t::dial, "Topics"},
	{tools_t::rec_type_t::info, "Dialogues"},
	{tools_t::rec_type_t::fnam, "Names"},
	{tools_t::rec_type_t::text, "Books"},
	{tools_t::rec_type_t::gmst, "Settings"},
	{tools_t::rec_type_t::desc, "Descriptions"},
	{tools_t::rec_type_t::rnam, "Factions"},
	{tools_t::rec_type_t::indx, "Index"},
	{tools_t::rec_type_t::sctx, "Scripts"},
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

static const QColor color_selected_fg(90, 155, 230);
static const QColor color_deselected_fg(80, 80, 80);
static const QColor color_disabled_fg(180, 180, 180);

filter_tree_t::filter_tree_t(QWidget * parent)
	: QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	tree_ = new QTreeWidget(this);
	tree_->setHeaderHidden(true);
	tree_->setColumnCount(2);
	tree_->setRootIsDecorated(true);
	tree_->setIndentation(16);
	tree_->setSelectionMode(QAbstractItemView::NoSelection);
	tree_->setFocusPolicy(Qt::NoFocus);
	layout->addWidget(tree_);

	all_item_ = new QTreeWidgetItem(tree_);
	all_item_->setText(0, "All");
	all_item_->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
	all_item_->setData(0, role_is_all, true);
	all_item_->setData(0, role_state, static_cast<int>(node_state_t::selected));

	auto add_sub_types = [this](QTreeWidgetItem * parent_item, tools_t::rec_type_t parent_type,
		const std::vector<std::string> & subs)
	{
		for (const auto & sub : subs)
		{
			auto * child = new QTreeWidgetItem(parent_item);
			child->setText(0, QString::fromStdString(sub));
			child->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
			child->setData(0, role_sub_type, QString::fromStdString(sub));
			child->setData(0, role_state, static_cast<int>(node_state_t::selected));

			sub_type_nodes_.push_back({child, sub, parent_type});
		}
	};

	for (const auto & [type, name] : type_order)
	{
		auto * item = new QTreeWidgetItem(all_item_);
		item->setText(0, name);
		item->setTextAlignment(1, Qt::AlignRight | Qt::AlignVCenter);
		item->setData(0, role_type, static_cast<int>(type));
		item->setData(0, role_state, static_cast<int>(node_state_t::selected));

		type_nodes_.push_back({item, type});

		if (type == tools_t::rec_type_t::info)
			add_sub_types(item, type, info_sub_types);
		else if (type == tools_t::rec_type_t::fnam)
			add_sub_types(item, type, fnam_sub_types);
		else if (type == tools_t::rec_type_t::desc)
			add_sub_types(item, type, desc_sub_types);
		else if (type == tools_t::rec_type_t::indx)
			add_sub_types(item, type, indx_sub_types);
	}

	yaml_item_ = new QTreeWidgetItem(tree_);
	yaml_item_->setText(0, "YAML");
	yaml_item_->setData(0, role_is_yaml, true);
	yaml_item_->setData(0, role_state, static_cast<int>(node_state_t::deselected));
	yaml_item_->setHidden(true);

	tree_->expandAll();
	tree_->header()->setStretchLastSection(false);
	tree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	tree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	update_item_styles();

	connect(tree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem * item, int) {
		on_item_right_clicked(item);
	});
	tree_->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(tree_, &QWidget::customContextMenuRequested, this, [this](const QPoint & pos) {
		auto * item = tree_->itemAt(pos);
		if (item)
			on_item_clicked(item, 0);
	});
}

void filter_tree_t::on_item_clicked(QTreeWidgetItem * item, int)
{
	if (!enabled_)
		return;

	if (item->data(0, role_is_all).toBool())
	{
		bool any_deselected = false;
		for (const auto & tn : type_nodes_)
		{
			auto state = static_cast<node_state_t>(tn.item->data(0, role_state).toInt());
			if (state != node_state_t::selected)
			{
				any_deselected = true;
				break;
			}
		}

		if (!any_deselected)
		{
			for (const auto & stn : sub_type_nodes_)
			{
				auto state = static_cast<node_state_t>(stn.item->data(0, role_state).toInt());
				if (state != node_state_t::selected)
				{
					any_deselected = true;
					break;
				}
			}
		}

		if (any_deselected)
		{
			for (auto & tn : type_nodes_)
				tn.item->setData(0, role_state, static_cast<int>(node_state_t::selected));

			for (auto & stn : sub_type_nodes_)
				stn.item->setData(0, role_state, static_cast<int>(node_state_t::selected));
		}
		else
		{
			for (auto & tn : type_nodes_)
				tn.item->setData(0, role_state, static_cast<int>(node_state_t::deselected));

			for (auto & stn : sub_type_nodes_)
				stn.item->setData(0, role_state, static_cast<int>(node_state_t::deselected));
		}

		update_all_node_state();
		update_item_styles();
		emit all_reset_requested();
		emit filters_changed();
		return;
	}

	if (item->data(0, role_is_yaml).toBool())
	{
		auto state = static_cast<node_state_t>(item->data(0, role_state).toInt());
		bool new_selected = (state != node_state_t::selected);
		item->setData(0, role_state, static_cast<int>(new_selected ? node_state_t::selected : node_state_t::deselected));
		update_item_styles();
		emit filters_changed();
		return;
	}

	if (item->data(0, role_sub_type).isValid())
	{
		auto state = static_cast<node_state_t>(item->data(0, role_state).toInt());
		bool new_selected = (state != node_state_t::selected);
		item->setData(0, role_state, static_cast<int>(new_selected ? node_state_t::selected : node_state_t::deselected));
		update_parent_state(item->parent());
		update_all_node_state();
		update_item_styles();
		emit filters_changed();
		return;
	}

	if (item->data(0, role_type).isValid())
	{
		if (item->childCount() == 0)
		{
			auto state = static_cast<node_state_t>(item->data(0, role_state).toInt());
			bool new_selected = (state != node_state_t::selected);
			item->setData(0, role_state, static_cast<int>(new_selected ? node_state_t::selected : node_state_t::deselected));
		}
		else
		{
			bool any_child_deselected = false;
			for (int i = 0; i < item->childCount(); ++i)
			{
				auto child_state = static_cast<node_state_t>(item->child(i)->data(0, role_state).toInt());
				if (child_state != node_state_t::selected)
				{
					any_child_deselected = true;
					break;
				}
			}

			node_state_t new_child_state = any_child_deselected ? node_state_t::selected : node_state_t::deselected;
			for (int i = 0; i < item->childCount(); ++i)
				item->child(i)->setData(0, role_state, static_cast<int>(new_child_state));

			update_parent_state(item);
		}

		update_all_node_state();
		update_item_styles();
		emit filters_changed();
	}
}

void filter_tree_t::on_item_right_clicked(QTreeWidgetItem * item)
{
	if (!enabled_)
		return;

	if (item->data(0, role_is_all).toBool())
	{
		for (auto & tn : type_nodes_)
			tn.item->setData(0, role_state, static_cast<int>(node_state_t::selected));

		for (auto & stn : sub_type_nodes_)
			stn.item->setData(0, role_state, static_cast<int>(node_state_t::selected));

		update_all_node_state();
		update_item_styles();
		emit all_reset_requested();
		emit filters_changed();
		return;
	}

	if (item->data(0, role_is_yaml).toBool())
		return;

	for (auto & tn : type_nodes_)
		tn.item->setData(0, role_state, static_cast<int>(node_state_t::deselected));

	for (auto & stn : sub_type_nodes_)
		stn.item->setData(0, role_state, static_cast<int>(node_state_t::deselected));

	if (item->data(0, role_sub_type).isValid())
	{
		item->setData(0, role_state, static_cast<int>(node_state_t::selected));
		update_parent_state(item->parent());
	}
	else if (item->data(0, role_type).isValid())
	{
		if (item->childCount() == 0)
		{
			item->setData(0, role_state, static_cast<int>(node_state_t::selected));
		}
		else
		{
			for (int i = 0; i < item->childCount(); ++i)
				item->child(i)->setData(0, role_state, static_cast<int>(node_state_t::selected));

			update_parent_state(item);
		}
	}

	update_all_node_state();
	update_item_styles();
	emit filters_changed();
}

void filter_tree_t::update_parent_state(QTreeWidgetItem * parent)
{
	if (!parent)
		return;

	int selected_count = 0;
	int total = parent->childCount();

	for (int i = 0; i < total; ++i)
	{
		auto state = static_cast<node_state_t>(parent->child(i)->data(0, role_state).toInt());
		if (state == node_state_t::selected)
			++selected_count;
	}

	node_state_t new_state;
	if (selected_count == 0)
		new_state = node_state_t::deselected;
	else if (selected_count == total)
		new_state = node_state_t::selected;
	else
		new_state = node_state_t::partial;

	parent->setData(0, role_state, static_cast<int>(new_state));
}

void filter_tree_t::update_all_node_state()
{
	int selected_count = 0;
	int total = static_cast<int>(type_nodes_.size());

	for (const auto & tn : type_nodes_)
	{
		auto state = static_cast<node_state_t>(tn.item->data(0, role_state).toInt());
		if (state == node_state_t::selected)
			++selected_count;
	}

	node_state_t new_state;
	if (selected_count == 0)
		new_state = node_state_t::deselected;
	else if (selected_count == total)
		new_state = node_state_t::selected;
	else
		new_state = node_state_t::partial;

	all_item_->setData(0, role_state, static_cast<int>(new_state));
}

void filter_tree_t::update_item_styles()
{
	apply_item_style(all_item_);
	apply_item_style(yaml_item_);

	for (const auto & tn : type_nodes_)
		apply_item_style(tn.item);

	for (const auto & stn : sub_type_nodes_)
		apply_item_style(stn.item);
}

void filter_tree_t::apply_item_style(QTreeWidgetItem * item)
{
	if (!enabled_)
	{
		item->setForeground(0, QBrush(color_disabled_fg));
		item->setForeground(1, QBrush(color_disabled_fg));
		return;
	}

	auto state = static_cast<node_state_t>(item->data(0, role_state).toInt());

	switch (state)
	{
	case node_state_t::selected:
		item->setBackground(0, QBrush(color_selected_fg));
		item->setForeground(0, QBrush(QColor(255, 255, 255)));
		item->setBackground(1, QBrush(color_selected_fg));
		item->setForeground(1, QBrush(QColor(255, 255, 255)));
		break;
	case node_state_t::partial:
		item->setBackground(0, QBrush(QColor(160, 195, 235)));
		item->setForeground(0, QBrush(QColor(255, 255, 255)));
		item->setBackground(1, QBrush(QColor(160, 195, 235)));
		item->setForeground(1, QBrush(QColor(255, 255, 255)));
		break;
	case node_state_t::deselected:
		item->setBackground(0, QBrush());
		item->setForeground(0, QBrush(color_deselected_fg));
		item->setBackground(1, QBrush());
		item->setForeground(1, QBrush(color_deselected_fg));
		break;
	}
}

void filter_tree_t::update_counts(
	const std::map<tools_t::rec_type_t, size_t> & total_counts,
	const std::map<tools_t::rec_type_t, size_t> & translated_counts)
{
	for (auto & tn : type_nodes_)
	{
		auto total_it = total_counts.find(tn.type);
		size_t total = (total_it != total_counts.end()) ? total_it->second : 0;

		auto trans_it = translated_counts.find(tn.type);
		size_t translated = (trans_it != translated_counts.end()) ? trans_it->second : 0;

		tn.item->setHidden(total == 0);
		tn.item->setText(1, QString("%1/%2").arg(translated).arg(total));
	}
}

void filter_tree_t::update_sub_type_counts(
	const std::map<std::string, size_t> & sub_type_total_counts,
	const std::map<std::string, size_t> & sub_type_translated_counts)
{
	for (auto & stn : sub_type_nodes_)
	{
		auto total_it = sub_type_total_counts.find(stn.sub_type);
		size_t total = (total_it != sub_type_total_counts.end()) ? total_it->second : 0;

		auto trans_it = sub_type_translated_counts.find(stn.sub_type);
		size_t translated = (trans_it != sub_type_translated_counts.end()) ? trans_it->second : 0;

		stn.item->setHidden(total == 0);
		stn.item->setText(1, QString("%1/%2").arg(translated).arg(total));
	}
}

void filter_tree_t::set_total_count(size_t translated, size_t total)
{
	all_item_->setText(1, QString("%1/%2").arg(translated).arg(total));
}

std::set<tools_t::rec_type_t> filter_tree_t::get_active_types() const
{
	std::set<tools_t::rec_type_t> result;

	for (const auto & tn : type_nodes_)
	{
		if (tn.item->childCount() == 0)
		{
			auto state = static_cast<node_state_t>(tn.item->data(0, role_state).toInt());
			if (state == node_state_t::selected)
				result.insert(tn.type);
		}
		else
		{
			auto state = static_cast<node_state_t>(tn.item->data(0, role_state).toInt());
			if (state != node_state_t::deselected)
				result.insert(tn.type);
		}
	}

	return result;
}

std::set<std::string> filter_tree_t::get_active_sub_types() const
{
	std::set<std::string> result;

	for (const auto & stn : sub_type_nodes_)
	{
		auto state = static_cast<node_state_t>(stn.item->data(0, role_state).toInt());
		if (state == node_state_t::selected)
			result.insert(stn.sub_type);
	}

	return result;
}

void filter_tree_t::set_active_types(const std::set<tools_t::rec_type_t> & types)
{
	for (auto & tn : type_nodes_)
	{
		bool active = types.count(tn.type) > 0;
		tn.item->setData(0, role_state, static_cast<int>(active ? node_state_t::selected : node_state_t::deselected));

		for (int i = 0; i < tn.item->childCount(); ++i)
			tn.item->child(i)->setData(0, role_state, static_cast<int>(active ? node_state_t::selected : node_state_t::deselected));
	}

	update_all_node_state();
	update_item_styles();
}

void filter_tree_t::set_active_sub_types(const std::set<std::string> & sub_types)
{
	for (auto & stn : sub_type_nodes_)
	{
		bool active = sub_types.empty() || sub_types.count(stn.sub_type) > 0;
		stn.item->setData(0, role_state, static_cast<int>(active ? node_state_t::selected : node_state_t::deselected));
	}

	for (auto & tn : type_nodes_)
	{
		if (tn.item->childCount() > 0)
			update_parent_state(tn.item);
	}

	update_all_node_state();
	update_item_styles();
}

bool filter_tree_t::is_solo() const
{
	return false;
}

tools_t::rec_type_t filter_tree_t::get_solo_type() const
{
	return tools_t::rec_type_t::unknown;
}

bool filter_tree_t::is_yaml_filter_active() const
{
	auto state = static_cast<node_state_t>(yaml_item_->data(0, role_state).toInt());
	return state == node_state_t::selected;
}

void filter_tree_t::set_yaml_filter_active(bool active)
{
	yaml_item_->setData(0, role_state, static_cast<int>(active ? node_state_t::selected : node_state_t::deselected));
	apply_item_style(yaml_item_);
}

void filter_tree_t::set_yaml_button_visible(bool visible)
{
	yaml_item_->setHidden(!visible);
}

void filter_tree_t::setEnabled(bool enabled)
{
	enabled_ = enabled;
	QWidget::setEnabled(enabled);
	update_item_styles();
}

void filter_tree_t::set_display_mode(display_mode_t mode)
{
	switch (mode)
	{
	case display_mode_t::empty:
		all_item_->setHidden(true);
		yaml_item_->setHidden(true);
		for (auto & tn : type_nodes_)
			tn.item->setHidden(true);
		break;

	case display_mode_t::all_only:
		all_item_->setHidden(false);
		yaml_item_->setHidden(true);
		for (auto & tn : type_nodes_)
			tn.item->setHidden(true);
		break;

	case display_mode_t::full:
		all_item_->setHidden(false);
		yaml_item_->setHidden(true);
		for (auto & tn : type_nodes_)
			tn.item->setHidden(false);
		break;
	}
}
