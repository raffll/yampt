#pragma once

#include "../yampt/plugin_scan/conflict_types.hpp"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <set>
#include <string>
#include <vector>

class filter_dialog_t : public QDialog
{
	Q_OBJECT

public:
	explicit filter_dialog_t(const std::vector<std::string> & available_types, QWidget * parent = nullptr);

	struct filter_state_t
	{
		bool filter_conflict_all = false;
		std::set<conflict_all_t> conflict_all_set;

		bool filter_conflict_this = false;
		std::set<conflict_this_t> conflict_this_set;

		bool filter_by_type = false;
		std::set<std::string> type_set;

		bool filter_by_id = false;
		std::string id_text;

		bool filter_by_name = false;
		std::string name_text;

		bool filter_deleted = false;
		bool filter_itm_only = false;
	};

	filter_state_t state() const;
	void set_state(const filter_state_t & state);

private:
	QGroupBox * grp_conflict_all_ = nullptr;
	QCheckBox * chk_ca_only_one_ = nullptr;
	QCheckBox * chk_ca_no_conflict_ = nullptr;
	QCheckBox * chk_ca_override_ = nullptr;
	QCheckBox * chk_ca_conflict_ = nullptr;

	QGroupBox * grp_conflict_this_ = nullptr;
	QCheckBox * chk_ct_master_ = nullptr;
	QCheckBox * chk_ct_identical_ = nullptr;
	QCheckBox * chk_ct_override_ = nullptr;
	QCheckBox * chk_ct_wins_ = nullptr;
	QCheckBox * chk_ct_loses_ = nullptr;
	QCheckBox * chk_ct_deleted_ = nullptr;

	QCheckBox * chk_by_type_ = nullptr;
	QListWidget * lst_types_ = nullptr;

	QCheckBox * chk_by_id_ = nullptr;
	QLineEdit * edt_id_ = nullptr;

	QCheckBox * chk_by_name_ = nullptr;
	QLineEdit * edt_name_ = nullptr;

	QCheckBox * chk_deleted_ = nullptr;
	QCheckBox * chk_itm_ = nullptr;
};
