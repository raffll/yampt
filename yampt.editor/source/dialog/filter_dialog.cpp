#include "filter_dialog.hpp"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

filter_dialog_t::filter_dialog_t(const std::vector<std::string> & available_types, QWidget * parent)
    : QDialog(parent)
{
	setWindowTitle(tr("Advanced Filter"));
	setModal(true);
	resize(560, 500);

	auto * outer_layout = new QVBoxLayout(this);
	auto * columns_layout = new QHBoxLayout();

	auto * left_column = new QVBoxLayout();

	m_grp_conflict_all = new QGroupBox(tr("Conflict All"), this);
	auto * ca_layout = new QVBoxLayout(m_grp_conflict_all);
	m_chk_ca_only_one = new QCheckBox(tr("Only One"), m_grp_conflict_all);
	m_chk_ca_no_conflict = new QCheckBox(tr("No Conflict"), m_grp_conflict_all);
	m_chk_ca_override = new QCheckBox(tr("Override"), m_grp_conflict_all);
	m_chk_ca_conflict = new QCheckBox(tr("Conflict"), m_grp_conflict_all);
	ca_layout->addWidget(m_chk_ca_only_one);
	ca_layout->addWidget(m_chk_ca_no_conflict);
	ca_layout->addWidget(m_chk_ca_override);
	ca_layout->addWidget(m_chk_ca_conflict);
	left_column->addWidget(m_grp_conflict_all);

	m_grp_conflict_this = new QGroupBox(tr("Conflict This"), this);
	auto * ct_layout = new QVBoxLayout(m_grp_conflict_this);
	m_chk_ct_master = new QCheckBox(tr("Master"), m_grp_conflict_this);
	m_chk_ct_identical = new QCheckBox(tr("Identical to Master"), m_grp_conflict_this);
	m_chk_ct_override = new QCheckBox(tr("Override Wins"), m_grp_conflict_this);
	m_chk_ct_wins = new QCheckBox(tr("Conflict Wins"), m_grp_conflict_this);
	m_chk_ct_loses = new QCheckBox(tr("Conflict Loses"), m_grp_conflict_this);
	ct_layout->addWidget(m_chk_ct_master);
	ct_layout->addWidget(m_chk_ct_identical);
	ct_layout->addWidget(m_chk_ct_override);
	ct_layout->addWidget(m_chk_ct_wins);
	ct_layout->addWidget(m_chk_ct_loses);
	left_column->addWidget(m_grp_conflict_this);

	auto * grp_id_name = new QGroupBox(tr("ID / Name"), this);
	auto * id_name_layout = new QFormLayout(grp_id_name);
	m_edt_id = new QLineEdit(grp_id_name);
	m_edt_id->setPlaceholderText(tr("Substring, case-insensitive"));
	m_edt_name = new QLineEdit(grp_id_name);
	m_edt_name->setPlaceholderText(tr("Substring, case-insensitive"));
	id_name_layout->addRow(tr("ID:"), m_edt_id);
	id_name_layout->addRow(tr("Name:"), m_edt_name);
	left_column->addWidget(grp_id_name);

	auto * grp_special = new QGroupBox(tr("Special"), this);
	auto * special_layout = new QVBoxLayout(grp_special);
	m_chk_deleted = new QCheckBox(tr("Deleted Only"), grp_special);
	special_layout->addWidget(m_chk_deleted);
	left_column->addWidget(grp_special);

	left_column->addStretch();

	auto * right_column = new QVBoxLayout();

	auto * grp_type = new QGroupBox(tr("Record Type"), this);
	auto * type_layout = new QVBoxLayout(grp_type);
	m_lst_types = new QListWidget(grp_type);
	for (const auto & t : available_types)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(t));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		m_lst_types->addItem(item);
	}
	type_layout->addWidget(m_lst_types);
	right_column->addWidget(grp_type);

	setup_lua_group(right_column);

	columns_layout->addLayout(left_column);
	columns_layout->addLayout(right_column);

	outer_layout->addLayout(columns_layout);

	auto * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	outer_layout->addWidget(buttons);

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void filter_dialog_t::setup_lua_group(QVBoxLayout * parent_layout)
{
	m_grp_lua = new QGroupBox(tr("Lua Handlers"), this);
	auto * lua_layout = new QVBoxLayout(m_grp_lua);

	m_chk_lua_blocking = new QCheckBox(tr("Blocking"), m_grp_lua);
	m_chk_lua_blocking->setChecked(true);
	m_chk_lua_mutating = new QCheckBox(tr("Mutating"), m_grp_lua);
	m_chk_lua_mutating->setChecked(true);
	m_chk_lua_overlapping = new QCheckBox(tr("Overlapping"), m_grp_lua);
	m_chk_lua_overlapping->setChecked(true);

	lua_layout->addWidget(m_chk_lua_blocking);
	lua_layout->addWidget(m_chk_lua_mutating);
	lua_layout->addWidget(m_chk_lua_overlapping);

	auto * label_interfaces = new QLabel(tr("Interfaces:"), m_grp_lua);
	lua_layout->addWidget(label_interfaces);

	m_lst_lua_interfaces = new QListWidget(m_grp_lua);
	m_lst_lua_interfaces->setSelectionMode(QAbstractItemView::MultiSelection);
	lua_layout->addWidget(m_lst_lua_interfaces);

	m_grp_lua->setVisible(false);
	parent_layout->addWidget(m_grp_lua);
}

filter_dialog_t::filter_state_t filter_dialog_t::state() const
{
	filter_state_t s;

	if (m_chk_ca_only_one->isChecked())
		s.conflict_all_set.insert(conflict_all_t::only_one);
	if (m_chk_ca_no_conflict->isChecked())
		s.conflict_all_set.insert(conflict_all_t::no_conflict);
	if (m_chk_ca_override->isChecked())
		s.conflict_all_set.insert(conflict_all_t::override_benign);
	if (m_chk_ca_conflict->isChecked())
		s.conflict_all_set.insert(conflict_all_t::conflict);
	s.filter_conflict_all = !s.conflict_all_set.empty();

	if (m_chk_ct_master->isChecked())
		s.conflict_this_set.insert(conflict_this_t::master);
	if (m_chk_ct_identical->isChecked())
		s.conflict_this_set.insert(conflict_this_t::identical_to_master);
	if (m_chk_ct_override->isChecked())
		s.conflict_this_set.insert(conflict_this_t::override_wins);
	if (m_chk_ct_wins->isChecked())
		s.conflict_this_set.insert(conflict_this_t::conflict_wins);
	if (m_chk_ct_loses->isChecked())
		s.conflict_this_set.insert(conflict_this_t::conflict_loses);
	s.filter_conflict_this = !s.conflict_this_set.empty();

	for (int i = 0; i < m_lst_types->count(); ++i)
	{
		const auto * item = m_lst_types->item(i);
		if (item->checkState() == Qt::Checked)
			s.type_set.insert(item->text().toStdString());
	}
	s.filter_by_type = !s.type_set.empty() && static_cast<int>(s.type_set.size()) < m_lst_types->count();

	s.id_text = m_edt_id->text().toStdString();
	s.filter_by_id = !s.id_text.empty();

	s.name_text = m_edt_name->text().toStdString();
	s.filter_by_name = !s.name_text.empty();

	s.filter_deleted = m_chk_deleted->isChecked();

	s.lua_severity_set = read_lua_severity_state();
	s.filter_lua_severity = !s.lua_severity_set.empty() && s.lua_severity_set.size() < 3;

	s.lua_interface_set = read_lua_interface_state();
	s.filter_lua_interface =
	    !s.lua_interface_set.empty() && static_cast<int>(s.lua_interface_set.size()) < m_lst_lua_interfaces->count();

	return s;
}

void filter_dialog_t::set_state(const filter_state_t & state)
{
	m_chk_ca_only_one->setChecked(state.conflict_all_set.count(conflict_all_t::only_one) > 0);
	m_chk_ca_no_conflict->setChecked(state.conflict_all_set.count(conflict_all_t::no_conflict) > 0);
	m_chk_ca_override->setChecked(state.conflict_all_set.count(conflict_all_t::override_benign) > 0);
	m_chk_ca_conflict->setChecked(state.conflict_all_set.count(conflict_all_t::conflict) > 0);

	m_chk_ct_master->setChecked(state.conflict_this_set.count(conflict_this_t::master) > 0);
	m_chk_ct_identical->setChecked(state.conflict_this_set.count(conflict_this_t::identical_to_master) > 0);
	m_chk_ct_override->setChecked(state.conflict_this_set.count(conflict_this_t::override_wins) > 0);
	m_chk_ct_wins->setChecked(state.conflict_this_set.count(conflict_this_t::conflict_wins) > 0);
	m_chk_ct_loses->setChecked(state.conflict_this_set.count(conflict_this_t::conflict_loses) > 0);

	for (int i = 0; i < m_lst_types->count(); ++i)
	{
		auto * item = m_lst_types->item(i);
		const auto type_str = item->text().toStdString();
		item->setCheckState(state.type_set.count(type_str) > 0 ? Qt::Checked : Qt::Unchecked);
	}

	m_edt_id->setText(QString::fromStdString(state.id_text));
	m_edt_name->setText(QString::fromStdString(state.name_text));

	m_chk_deleted->setChecked(state.filter_deleted);

	apply_lua_severity_state(state.lua_severity_set);
	apply_lua_interface_state(state.lua_interface_set);
}

void filter_dialog_t::set_lua_interface_names(const std::vector<std::string> & names)
{
	m_lst_lua_interfaces->clear();

	if (names.empty())
	{
		m_grp_lua->setVisible(false);
		return;
	}

	for (const auto & name : names)
		m_lst_lua_interfaces->addItem(QString::fromStdString(name));

	m_grp_lua->setVisible(true);
}

std::set<conflict_severity_t> filter_dialog_t::read_lua_severity_state() const
{
	std::set<conflict_severity_t> result;

	if (m_chk_lua_blocking->isChecked())
		result.insert(conflict_severity_t::blocking);

	if (m_chk_lua_mutating->isChecked())
		result.insert(conflict_severity_t::mutating);

	if (m_chk_lua_overlapping->isChecked())
		result.insert(conflict_severity_t::overlapping);

	return result;
}

std::set<std::string> filter_dialog_t::read_lua_interface_state() const
{
	std::set<std::string> result;

	for (const auto * item : m_lst_lua_interfaces->selectedItems())
		result.insert(item->text().toStdString());

	return result;
}

void filter_dialog_t::apply_lua_severity_state(const std::set<conflict_severity_t> & severities)
{
	const bool has_filter = !severities.empty();
	m_chk_lua_blocking->setChecked(!has_filter || severities.count(conflict_severity_t::blocking) > 0);
	m_chk_lua_mutating->setChecked(!has_filter || severities.count(conflict_severity_t::mutating) > 0);
	m_chk_lua_overlapping->setChecked(!has_filter || severities.count(conflict_severity_t::overlapping) > 0);
}

void filter_dialog_t::apply_lua_interface_state(const std::set<std::string> & interfaces)
{
	for (int i = 0; i < m_lst_lua_interfaces->count(); ++i)
	{
		auto * item = m_lst_lua_interfaces->item(i);
		const bool selected = interfaces.count(item->text().toStdString()) > 0;
		item->setSelected(selected);
	}
}
