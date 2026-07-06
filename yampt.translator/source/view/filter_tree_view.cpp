#include "filter_tree_view.hpp"
#include <utility/record_types.hpp>
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

static const std::vector<std::pair<rec_type_t, const char *>> type_order = {
	{ rec_type_t::cell, "Cells" },        { rec_type_t::dial, "Topics" },
	{ rec_type_t::info, "Dialogues" },    { rec_type_t::fnam, "Names" },
	{ rec_type_t::text, "Books" },        { rec_type_t::gmst, "Settings" },
	{ rec_type_t::desc, "Descriptions" }, { rec_type_t::rnam, "Factions" },
	{ rec_type_t::indx, "Index" },        { rec_type_t::sctx, "Scripts" },
};

static const QColor color_selected_bg(90, 155, 230);
static const QColor color_selected_fg(255, 255, 255);
static const QColor color_deselected_fg(80, 80, 80);
static const QColor color_disabled_fg(180, 180, 180);

filter_tree_view_t::filter_tree_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_list = new QListWidget(this);
	m_list->setSelectionMode(QAbstractItemView::NoSelection);
	m_list->setFocusPolicy(Qt::NoFocus);
	m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_list->setItemDelegate(new filter_delegate_t(m_list));
	layout->addWidget(m_list);

	build_rows();
	update_styles();

	connect(m_list, &QListWidget::itemClicked, this, &filter_tree_view_t::on_item_clicked);
	m_list->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(
	    m_list,
	    &QWidget::customContextMenuRequested,
	    this,
	    [this](const QPoint & pos)
	{
		auto * item = m_list->itemAt(pos);
		if (item)
			on_item_right_clicked(item);
	});
}

void filter_tree_view_t::build_rows()
{
	auto add_row =
	    [this](item_kind_t kind, rec_type_t type, const std::string & sub_type, const char * label) -> int
	{
		auto * item = new QListWidgetItem(label, m_list);
		item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		int idx = static_cast<int>(m_rows.size());
		m_rows.push_back({ item, kind, type, sub_type, true });
		return idx;
	};

	m_all_row = add_row(item_kind_t::all, rec_type_t::unknown, {}, "All");

	for (const auto & [type, name] : type_order)
	{
		if (type == rec_type_t::info)
		{
			for (const auto & entry : record_types::info_sub_types)
			{
				auto label = std::string("Dialogues: ") + std::string(entry.display_name);
				add_row(item_kind_t::sub_type, type, std::string(entry.display_name), label.c_str());
			}
		}
		else if (type == rec_type_t::fnam)
		{
			for (const auto & entry : record_types::fnam_sub_types)
			{
				const auto display = record_types::fnam_display_name(entry.prefix);
				auto label = std::string("Names: ") + std::string(display);
				add_row(item_kind_t::sub_type, type, std::string(entry.display_name), label.c_str());
			}
		}
		else if (type == rec_type_t::desc)
		{
			for (const auto & entry : record_types::desc_sub_types)
			{
				auto label = std::string("Descriptions: ") + std::string(entry.display_name);
				add_row(item_kind_t::sub_type, type, std::string(entry.display_name), label.c_str());
			}
		}
		else if (type == rec_type_t::indx)
		{
			for (const auto & entry : record_types::indx_sub_types)
			{
				auto label = std::string("Index: ") + std::string(entry.display_name);
				add_row(item_kind_t::sub_type, type, std::string(entry.display_name), label.c_str());
			}
		}
		else
		{
			add_row(item_kind_t::type, type, {}, name);
		}
	}

	m_yaml_row = add_row(item_kind_t::yaml, rec_type_t::unknown, {}, "YAML");
	m_rows[m_yaml_row].selected = false;
	m_rows[m_yaml_row].item->setHidden(true);
}

void filter_tree_view_t::on_item_clicked(QListWidgetItem * item)
{
	if (!m_enabled)
		return;

	int idx = m_list->row(item);
	if (idx < 0 || idx >= static_cast<int>(m_rows.size()))
		return;

	auto & row = m_rows[idx];

	if (row.kind == item_kind_t::all)
	{
		for (auto & r : m_rows)
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

	for (auto & r : m_rows)
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

void filter_tree_view_t::on_item_right_clicked(QListWidgetItem * item)
{
	if (!m_enabled)
		return;

	int idx = m_list->row(item);
	if (idx < 0 || idx >= static_cast<int>(m_rows.size()))
		return;

	auto & row = m_rows[idx];

	if (row.kind == item_kind_t::all)
	{
		bool any_deselected = false;
		for (const auto & r : m_rows)
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
		for (auto & r : m_rows)
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

void filter_tree_view_t::update_all_state()
{
	if (m_all_row < 0)
		return;

	bool all_selected = true;
	for (const auto & r : m_rows)
	{
		if (r.kind == item_kind_t::all || r.kind == item_kind_t::yaml)
			continue;

		if (!r.selected)
		{
			all_selected = false;
			break;
		}
	}

	m_rows[m_all_row].selected = all_selected;
}

void filter_tree_view_t::update_styles()
{
	for (auto & r : m_rows)
	{
		if (!m_enabled)
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

void filter_tree_view_t::update_counts(
    const std::map<rec_type_t, size_t> & total_counts,
    const std::map<rec_type_t, size_t> & translated_counts)
{
	for (auto & r : m_rows)
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

void filter_tree_view_t::update_sub_type_counts(
    const std::map<std::string, size_t> & sub_type_total_counts,
    const std::map<std::string, size_t> & sub_type_translated_counts)
{
	for (auto & r : m_rows)
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

void filter_tree_view_t::set_total_count(size_t translated, size_t total)
{
	if (m_all_row < 0)
		return;

	m_rows[m_all_row].item->setData(role_counter, QString("%1/%2").arg(translated).arg(total));
}

std::set<rec_type_t> filter_tree_view_t::get_active_types() const
{
	std::set<rec_type_t> result;

	for (const auto & r : m_rows)
	{
		if (r.kind == item_kind_t::type && r.selected)
			result.insert(r.type);

		if (r.kind == item_kind_t::sub_type && r.selected)
			result.insert(r.type);
	}

	return result;
}

std::set<std::string> filter_tree_view_t::get_active_sub_types() const
{
	std::set<std::string> result;

	for (const auto & r : m_rows)
	{
		if (r.kind == item_kind_t::sub_type && r.selected)
			result.insert(r.sub_type);
	}

	return result;
}

bool filter_tree_view_t::has_sub_type_filter() const
{
	for (const auto & r : m_rows)
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

void filter_tree_view_t::set_active_types(const std::set<rec_type_t> & types)
{
	for (auto & r : m_rows)
	{
		if (r.kind == item_kind_t::all || r.kind == item_kind_t::yaml)
			continue;

		r.selected = types.count(r.type) > 0;
	}

	update_all_state();
	update_styles();
}

void filter_tree_view_t::set_active_sub_types(const std::set<std::string> & sub_types)
{
	for (auto & r : m_rows)
	{
		if (r.kind != item_kind_t::sub_type)
			continue;

		r.selected = sub_types.empty() || sub_types.count(r.sub_type) > 0;
	}

	update_all_state();
	update_styles();
}

bool filter_tree_view_t::is_solo() const
{
	int active_count = 0;
	for (const auto & r : m_rows)
	{
		if (r.kind == item_kind_t::all || r.kind == item_kind_t::yaml)
			continue;

		if (r.selected)
			++active_count;
	}

	return active_count == 1;
}

rec_type_t filter_tree_view_t::get_solo_type() const
{
	for (const auto & r : m_rows)
	{
		if (r.kind == item_kind_t::all || r.kind == item_kind_t::yaml)
			continue;

		if (r.selected)
			return r.type;
	}

	return rec_type_t::unknown;
}

bool filter_tree_view_t::is_yaml_filter_active() const
{
	if (m_yaml_row < 0)
		return false;

	return m_rows[m_yaml_row].selected;
}

void filter_tree_view_t::set_yaml_filter_active(bool active)
{
	if (m_yaml_row < 0)
		return;

	m_rows[m_yaml_row].selected = active;
	update_styles();
}

void filter_tree_view_t::set_yaml_button_visible(bool visible)
{
	if (m_yaml_row < 0)
		return;

	m_rows[m_yaml_row].item->setHidden(!visible);
}

void filter_tree_view_t::setEnabled(bool enabled)
{
	m_enabled = enabled;
	QWidget::setEnabled(enabled);
	update_styles();
}

void filter_tree_view_t::set_display_mode(display_mode_t mode)
{
	switch (mode)
	{
	case display_mode_t::empty:
		for (auto & r : m_rows)
			r.item->setHidden(true);
		break;

	case display_mode_t::all_only:
		for (auto & r : m_rows)
			r.item->setHidden(r.kind != item_kind_t::all);
		break;

	case display_mode_t::full:
		for (auto & r : m_rows)
		{
			if (r.kind == item_kind_t::yaml)
				continue;

			r.item->setHidden(false);
		}
		break;
	}
}
