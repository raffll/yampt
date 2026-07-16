#include "status_filter_view.hpp"
#include "status_display.hpp"
#include <theme_system.hpp>
#include <QListWidget>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

static const int role_counter = Qt::UserRole + 101;
static const int role_bullet_color = Qt::UserRole + 200;

static const QColor color_selected_bg(90, 155, 230);
static const QColor color_selected_fg(255, 255, 255);
static const QColor color_deselected_fg(80, 80, 80);
static const QColor color_disabled_fg(180, 180, 180);

static const std::vector<status_t> status_order = {
	status_t::translated,
	status_t::to_verify,
	status_t::untranslated,
	status_t::reused,
	status_t::adapted,
	status_t::ambiguous,
	status_t::changed,
	status_t::outdated,
	status_t::duplicate,
	status_t::heuristic,
	status_t::missing,
	status_t::mismatch,
	status_t::in_progress,
	status_t::propagated,
	status_t::model,
	status_t::error,
};

class status_delegate_t : public QStyledItemDelegate
{
public:
	using QStyledItemDelegate::QStyledItemDelegate;

	void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const override
	{
		painter->save();

		if (option.state & QStyle::State_Selected)
			painter->fillRect(option.rect, option.palette.highlight());
		else if (index.data(Qt::BackgroundRole).canConvert<QBrush>())
			painter->fillRect(option.rect, index.data(Qt::BackgroundRole).value<QBrush>());

		const auto text = index.data(Qt::DisplayRole).toString();
		const auto bullet_color = index.data(role_bullet_color).value<QColor>();
		const auto text_color = index.data(Qt::ForegroundRole).value<QBrush>().color();
		const auto rect = option.rect.adjusted(6, 0, -6, 0);

		if (bullet_color.isValid() && text.startsWith(QString::fromUtf8("\xe2\x97\x8f")))
		{
			painter->setPen(bullet_color);
			const auto bullet_text = QString::fromUtf8("\xe2\x97\x8f ");
			painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, bullet_text);

			const auto bullet_width = painter->fontMetrics().horizontalAdvance(bullet_text);
			auto text_rect = rect.adjusted(bullet_width, 0, 0, 0);
			painter->setPen(text_color);
			painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, text.mid(2));
		}
		else
		{
			painter->setPen(text_color);
			painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
		}

		auto counter = index.data(role_counter).toString();
		if (!counter.isEmpty())
		{
			painter->setPen(text_color);
			painter->drawText(rect, Qt::AlignRight | Qt::AlignVCenter, counter);
		}

		painter->restore();
	}
};

status_filter_view_t::status_filter_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_list = new QListWidget(this);
	m_list->setSelectionMode(QAbstractItemView::NoSelection);
	m_list->setFocusPolicy(Qt::NoFocus);
	m_list->setMouseTracking(true);
	m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_list->setItemDelegate(new status_delegate_t(m_list));
	layout->addWidget(m_list);

	build_rows();
	update_styles();

	connect(m_list, &QListWidget::itemClicked, this, &status_filter_view_t::on_item_clicked);
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

static const char * get_status_tooltip(status_t status)
{
	switch (status)
	{
	case status_t::translated:
		return "Translation present and unchanged";
	case status_t::to_verify:
		return "Identical text, needs verification";
	case status_t::untranslated:
		return "No translation provided yet";
	case status_t::reused:
		return "Translation copied from another entry";
	case status_t::adapted:
		return "Translation adapted from a similar entry";
	case status_t::ambiguous:
		return "Multiple conflicting translations found in base dicts";
	case status_t::changed:
		return "Original text changed since last translation";
	case status_t::outdated:
		return "Source text changed while translation was in progress";
	case status_t::duplicate:
		return "Multiple records share the same key";
	case status_t::heuristic:
		return "Matched by heuristic (needs verification)";
	case status_t::missing:
		return "No matching record found in the other ESM";
	case status_t::mismatch:
		return "Record exists but original text differs";
	case status_t::in_progress:
		return "Translation edited but not finalized";
	case status_t::propagated:
		return "Translation propagated from another record";
	case status_t::model:
		return "Translated by the AI translation engine";
	case status_t::error:
		return "Translation has a validation error";
	}
	return "";
}

void status_filter_view_t::build_rows()
{
	auto * all_item = new QListWidgetItem("All", m_list);
	all_item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	m_all_row = 0;
	m_rows.push_back({ all_item, status_t::untranslated, true, true });

	for (const auto & status : status_order)
	{
		const auto & color = theme_system_t::instance().get_status_color(status);
		const auto bullet = QString::fromUtf8("\xe2\x97\x8f ");
		auto * item = new QListWidgetItem(bullet + status_display_name(status), m_list);
		item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		item->setToolTip(get_status_tooltip(status));
		item->setData(Qt::UserRole + 200, color);
		m_rows.push_back({ item, status, false, true });
	}
}

void status_filter_view_t::on_item_clicked(QListWidgetItem * item)
{
	if (!m_enabled)
		return;

	int index = m_list->row(item);
	if (index < 0 || index >= static_cast<int>(m_rows.size()))
		return;

	auto & row = m_rows[index];

	if (row.is_all)
	{
		for (auto & current_row : m_rows)
			current_row.selected = true;

		update_styles();
		emit filters_changed();
		return;
	}

	for (auto & current_row : m_rows)
	{
		if (current_row.is_all)
			continue;

		current_row.selected = false;
	}

	row.selected = true;
	update_all_state();
	update_styles();
	emit filters_changed();
}

void status_filter_view_t::on_item_right_clicked(QListWidgetItem * item)
{
	if (!m_enabled)
		return;

	int index = m_list->row(item);
	if (index < 0 || index >= static_cast<int>(m_rows.size()))
		return;

	auto & row = m_rows[index];

	if (row.is_all)
	{
		bool any_deselected = false;
		for (const auto & current_row : m_rows)
		{
			if (current_row.is_all)
				continue;

			if (!current_row.selected)
			{
				any_deselected = true;
				break;
			}
		}

		bool new_state = any_deselected;
		for (auto & current_row : m_rows)
		{
			if (current_row.is_all)
				continue;

			current_row.selected = new_state;
		}

		update_all_state();
		update_styles();
		emit filters_changed();
		return;
	}

	row.selected = !row.selected;
	update_all_state();
	update_styles();
	emit filters_changed();
}

void status_filter_view_t::update_all_state()
{
	if (m_all_row < 0)
		return;

	bool all_selected = true;
	for (const auto & current_row : m_rows)
	{
		if (current_row.is_all)
			continue;

		if (!current_row.selected)
		{
			all_selected = false;
			break;
		}
	}

	m_rows[m_all_row].selected = all_selected;
}

void status_filter_view_t::update_styles()
{
	const bool is_dark = theme_system_t::instance().active_theme() == theme_t::dark;

	for (auto & current_row : m_rows)
	{
		if (!m_enabled || !m_document_open)
		{
			current_row.item->setForeground(QBrush(color_disabled_fg));
			current_row.item->setBackground(QBrush());
			continue;
		}

		if (current_row.selected)
		{
			current_row.item->setBackground(QBrush(color_selected_bg));
			current_row.item->setForeground(QBrush(color_selected_fg));
		}
		else
		{
			current_row.item->setBackground(QBrush());
			const QColor deselected = is_dark ? QColor(120, 120, 120) : color_deselected_fg;
			current_row.item->setForeground(QBrush(deselected));
		}
	}
}

void status_filter_view_t::update_counts(
    const std::map<status_t, size_t> & /*displayed_counts*/,
    const std::map<status_t, size_t> & total_counts)
{
	size_t grand_total = 0;
	for (const auto & [status, count] : total_counts)
		grand_total += count;

	for (auto & current_row : m_rows)
	{
		if (current_row.is_all)
		{
			if (grand_total > 0)
				current_row.item->setData(role_counter, QString::number(grand_total));
			else
				current_row.item->setData(role_counter, QString());

			continue;
		}

		auto found = total_counts.find(current_row.status);
		size_t count = (found != total_counts.end()) ? found->second : 0;

		current_row.item->setHidden(m_visible_statuses.count(current_row.status) == 0);

		if (count > 0)
			current_row.item->setData(role_counter, QString::number(count));
		else
			current_row.item->setData(role_counter, QStringLiteral("0"));
	}
}

std::set<status_t> status_filter_view_t::get_active_statuses() const
{
	bool all_selected = (m_all_row >= 0) && m_rows[m_all_row].selected;
	if (all_selected)
		return {};

	std::set<status_t> result;
	for (const auto & current_row : m_rows)
	{
		if (current_row.is_all)
			continue;

		if (current_row.selected)
			result.insert(current_row.status);
	}

	return result;
}

bool status_filter_view_t::has_filter() const
{
	if (m_all_row >= 0 && m_rows[m_all_row].selected)
		return false;

	return true;
}

void status_filter_view_t::set_filter_state(const std::set<status_t> & statuses)
{
	if (statuses.empty())
	{
		for (auto & current_row : m_rows)
			current_row.selected = true;
	}
	else
	{
		for (auto & current_row : m_rows)
		{
			if (current_row.is_all)
				continue;

			current_row.selected = statuses.count(current_row.status) > 0;
		}

		update_all_state();
	}

	update_styles();
}

void status_filter_view_t::set_document_open(bool open)
{
	m_document_open = open;
	m_enabled = open;
	update_styles();
}

void status_filter_view_t::set_visible_statuses(const std::set<status_t> & visible)
{
	m_visible_statuses = visible;

	for (auto & current_row : m_rows)
	{
		if (current_row.is_all)
		{
			current_row.item->setHidden(visible.empty());
			continue;
		}

		current_row.item->setHidden(visible.count(current_row.status) == 0);
	}
}

void status_filter_view_t::refresh_theme()
{
	for (auto & current_row : m_rows)
	{
		if (current_row.is_all)
			continue;

		const auto & color = theme_system_t::instance().get_status_color(current_row.status);
		current_row.item->setData(role_bullet_color, color);
	}

	update_styles();
}
