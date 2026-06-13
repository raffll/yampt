#include "flow_layout.hpp"
#include <QWidget>

flow_layout_t::flow_layout_t(QWidget * parent, int margin, int h_spacing, int v_spacing)
    : QLayout(parent)
    , h_space_(h_spacing)
    , v_space_(v_spacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

flow_layout_t::~flow_layout_t()
{
    while (auto * item = takeAt(0))
        delete item;
}

void flow_layout_t::addItem(QLayoutItem * item)
{
    items_.append(item);
}

int flow_layout_t::count() const
{
    return items_.size();
}

QLayoutItem * flow_layout_t::itemAt(int index) const
{
    if (index >= 0 && index < items_.size())
        return items_.at(index);

    return nullptr;
}

QLayoutItem * flow_layout_t::takeAt(int index)
{
    if (index >= 0 && index < items_.size())
        return items_.takeAt(index);

    return nullptr;
}

Qt::Orientations flow_layout_t::expandingDirections() const
{
    return {};
}

bool flow_layout_t::hasHeightForWidth() const
{
    return true;
}

int flow_layout_t::heightForWidth(int width) const
{
    return do_layout(QRect(0, 0, width, 0), true);
}

QSize flow_layout_t::minimumSize() const
{
    QSize size;
    for (auto * item : items_)
        size = size.expandedTo(item->minimumSize());

    const auto margins = contentsMargins();
    size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
    return size;
}

QSize flow_layout_t::sizeHint() const
{
    return minimumSize();
}

void flow_layout_t::setGeometry(const QRect & rect)
{
    QLayout::setGeometry(rect);
    do_layout(rect, false);
}

int flow_layout_t::do_layout(const QRect & rect, bool test_only) const
{
    const auto margins = contentsMargins();
    const QRect effective = rect.adjusted(margins.left(), margins.top(), -margins.right(), -margins.bottom());

    int x = effective.x();
    int y = effective.y();
    int row_height = 0;

    for (auto * item : items_)
    {
        if (!item->widget() || item->widget()->isHidden())
            continue;

        const QSize item_size = item->sizeHint();
        int next_x = x + item_size.width() + h_space_;

        if (next_x - h_space_ > effective.right() && row_height > 0)
        {
            x = effective.x();
            y += row_height + v_space_;
            next_x = x + item_size.width() + h_space_;
            row_height = 0;
        }

        if (!test_only)
            item->setGeometry(QRect(QPoint(x, y), item_size));

        x = next_x;
        row_height = qMax(row_height, item_size.height());
    }

    return y + row_height - rect.y() + margins.bottom();
}
