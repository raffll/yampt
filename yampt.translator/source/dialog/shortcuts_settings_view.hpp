#pragma once

#include <string>
#include <vector>
#include <QWidget>

class QLabel;
class QPushButton;
class QTableWidget;
class app_settings_t;

class shortcuts_settings_view_t : public QWidget
{
    Q_OBJECT

public:
    explicit shortcuts_settings_view_t(QWidget * parent = nullptr);

    void load(const app_settings_t & settings);
    void apply(app_settings_t & settings) const;

    bool has_conflicts() const;

signals:
    void conflict_state_changed(bool has_conflicts);

private:
    struct shortcut_entry_t
    {
        std::string action_name;
        std::string display_name;
        std::string default_sequence;
    };

    void reset_to_defaults();
    void on_cell_double_clicked(int row, int column);
    void check_conflicts();

    QTableWidget * table_ = nullptr;
    QLabel * conflict_label_ = nullptr;
    QPushButton * reset_button_ = nullptr;
    std::vector<shortcut_entry_t> entries_;
    bool has_conflicts_ = false;
};
