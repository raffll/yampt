#pragma once

#include <scanner/conflict_types.hpp>
#include <set>
#include <string>
#include <vector>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>

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
	QGroupBox * m_grp_conflict_all = nullptr;
	QCheckBox * m_chk_ca_only_one = nullptr;
	QCheckBox * m_chk_ca_no_conflict = nullptr;
	QCheckBox * m_chk_ca_override = nullptr;
	QCheckBox * m_chk_ca_conflict = nullptr;

	QGroupBox * m_grp_conflict_this = nullptr;
	QCheckBox * m_chk_ct_master = nullptr;
	QCheckBox * m_chk_ct_identical = nullptr;
	QCheckBox * m_chk_ct_override = nullptr;
	QCheckBox * m_chk_ct_wins = nullptr;
	QCheckBox * m_chk_ct_loses = nullptr;

	QListWidget * m_lst_types = nullptr;

	QLineEdit * m_edt_id = nullptr;
	QLineEdit * m_edt_name = nullptr;

	QCheckBox * m_chk_deleted = nullptr;
	QCheckBox * m_chk_itm = nullptr;
};
