#pragma once

#include <scanner/conflict_detector.hpp>
#include <conflict_types.hpp>
#include <set>
#include <string>
#include <vector>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QListWidget>
#include <QVBoxLayout>

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

		bool filter_deleted = false;

		bool filter_lua_severity = false;
		std::set<conflict_severity_t> lua_severity_set;

		bool filter_lua_interface = false;
		std::set<std::string> lua_interface_set;
	};

	filter_state_t state() const;
	void set_state(const filter_state_t & state);
	void set_lua_interface_names(const std::vector<std::string> & names);

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

	QCheckBox * m_chk_deleted = nullptr;

	QGroupBox * m_grp_lua = nullptr;
	QCheckBox * m_chk_lua_blocking = nullptr;
	QCheckBox * m_chk_lua_mutating = nullptr;
	QCheckBox * m_chk_lua_overlapping = nullptr;
	QListWidget * m_lst_lua_interfaces = nullptr;

	void setup_lua_group(QVBoxLayout * parent_layout);
	std::set<conflict_severity_t> read_lua_severity_state() const;
	std::set<std::string> read_lua_interface_state() const;
	void apply_lua_severity_state(const std::set<conflict_severity_t> & severities);
	void apply_lua_interface_state(const std::set<std::string> & interfaces);
};
