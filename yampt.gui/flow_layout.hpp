#pragma once

#include <QLayout>
#include <QStyle>

class flow_layout_t : public QLayout
{
public:
    explicit flow_layout_t(QWidget * parent = nullptr, int margin = 0, int h_spacing = 2, int v_spacing = 2);
    ~flow_layout_t() override;

    void addItem(QLayoutItem * item) override;
    int count() const override;
    QLayoutItem * itemAt(int index) const override;
    QLayoutItem * takeAt(int index) override;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    QSize minimumSize() const override;
    QSize sizeHint() const override;
    void setGeometry(const QRect & rect) override;

private:
    int do_layout(const QRect & rect, bool test_only) const;

    QList<QLayoutItem *> items_;
    int h_space_;
    int v_space_;
};
