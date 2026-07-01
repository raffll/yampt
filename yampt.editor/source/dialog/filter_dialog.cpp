#include "filter_dialog.hpp"
#include <QVBoxLayout>

filter_dialog_t::filter_dialog_t(const std::vector<std::string> & available_types, QWidget * parent)
    : QDialog(parent)
{
	setWindowTitle("Advanced Filter");
	setModal(true);
	resize(400, 600);

	auto * main_layout = new QVBoxLayout(this);

	m_grp_conflict_all = new QGroupBox("Conflict All", this);
	auto * ca_layout = new QVBoxLayout(m_grp_conflict_all);
	m_chk_ca_only_one = new QCheckBox("Only One", m_grp_conflict_all);
	m_chk_ca_no_conflict = new QCheckBox("No Conflict", m_grp_conflict_all);
	m_chk_ca_override = new QCheckBox("Override", m_grp_conflict_all);
	m_chk_ca_conflict = new QCheckBox("Conflict", m_grp_conflict_all);
	ca_layout->addWidget(m_chk_ca_only_one);
	ca_layout->addWidget(m_chk_ca_no_conflict);
	ca_layout->addWidget(m_chk_ca_override);
	ca_layout->addWidget(m_chk_ca_conflict);
	main_layout->addWidget(m_grp_conflict_all);

	m_grp_conflict_this = new QGroupBox("Conflict This", this);
	auto * ct_layout = new QVBoxLayout(m_grp_conflict_this);
	m_chk_ct_master = new QCheckBox("Master", m_grp_conflict_this);
	m_chk_ct_identical = new QCheckBox("Identical to Master", m_grp_conflict_this);
	m_chk_ct_override = new QCheckBox("Override Wins", m_grp_conflict_this);
	m_chk_ct_wins = new QCheckBox("Conflict Wins", m_grp_conflict_this);
	m_chk_ct_loses = new QCheckBox("Conflict Loses", m_grp_conflict_this);
	m_chk_ct_deleted = new QCheckBox("Deleted", m_grp_conflict_this);
	ct_layout->addWidget(m_chk_ct_master);
	ct_layout->addWidget(m_chk_ct_identical);
	ct_layout->addWidget(m_chk_ct_override);
	ct_layout->addWidget(m_chk_ct_wins);
	ct_layout->addWidget(m_chk_ct_loses);
	ct_layout->addWidget(m_chk_ct_deleted);
	main_layout->addWidget(m_grp_conflict_this);

	auto * grp_type = new QGroupBox("Record Type", this);
	auto * type_layout = new QVBoxLayout(grp_type);
	m_chk_by_type = new QCheckBox("Filter by Type", grp_type);
	m_lst_types = new QListWidget(grp_type);
	for (const auto & t : available_types)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(t));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		m_lst_types->addItem(item);
	}
	type_layout->addWidget(m_chk_by_type);
	type_layout->addWidget(m_lst_types);
	main_layout->addWidget(grp_type);

	auto * grp_id_name = new QGroupBox("Editor ID / Name", this);
	auto * id_name_layout = new QVBoxLayout(grp_id_name);
	m_chk_by_id = new QCheckBox("By Editor ID", grp_id_name);
	m_edt_id = new QLineEdit(grp_id_name);
	m_edt_id->setPlaceholderText("Substring match (case-insensitive)");
	m_chk_by_name = new QCheckBox("By Display Name", grp_id_name);
	m_edt_name = new QLineEdit(grp_id_name);
	m_edt_name->setPlaceholderText("Substring match (case-insensitive)");
	id_name_layout->addWidget(m_chk_by_id);
	id_name_layout->addWidget(m_edt_id);
	id_name_layout->addWidget(m_chk_by_name);
	id_name_layout->addWidget(m_edt_name);
	main_layout->addWidget(grp_id_name);

	auto * grp_special = new QGroupBox("Special", this);
	auto * special_layout = new QVBoxLayout(grp_special);
	m_chk_deleted = new QCheckBox("Deleted only", grp_special);
	m_chk_itm = new QCheckBox("ITM only", grp_special);
	special_layout->addWidget(m_chk_deleted);
	special_layout->addWidget(m_chk_itm);
	main_layout->addWidget(grp_special);

	auto * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	main_layout->addWidget(buttons);

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
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
	if (m_chk_ct_deleted->isChecked())
		s.conflict_this_set.insert(conflict_this_t::deleted);
	s.filter_conflict_this = !s.conflict_this_set.empty();

	s.filter_by_type = m_chk_by_type->isChecked();
	if (s.filter_by_type)
	{
		for (int i = 0; i < m_lst_types->count(); ++i)
		{
			const auto * item = m_lst_types->item(i);
			if (item->checkState() == Qt::Checked)
				s.type_set.insert(item->text().toStdString());
		}
	}

	s.filter_by_id = m_chk_by_id->isChecked();
	s.id_text = m_edt_id->text().toStdString();

	s.filter_by_name = m_chk_by_name->isChecked();
	s.name_text = m_edt_name->text().toStdString();

	s.filter_deleted = m_chk_deleted->isChecked();
	s.filter_itm_only = m_chk_itm->isChecked();

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
	m_chk_ct_deleted->setChecked(state.conflict_this_set.count(conflict_this_t::deleted) > 0);

	m_chk_by_type->setChecked(state.filter_by_type);
	for (int i = 0; i < m_lst_types->count(); ++i)
	{
		auto * item = m_lst_types->item(i);
		const auto type_str = item->text().toStdString();
		item->setCheckState(state.type_set.count(type_str) > 0 ? Qt::Checked : Qt::Unchecked);
	}

	m_chk_by_id->setChecked(state.filter_by_id);
	m_edt_id->setText(QString::fromStdString(state.id_text));

	m_chk_by_name->setChecked(state.filter_by_name);
	m_edt_name->setText(QString::fromStdString(state.name_text));

	m_chk_deleted->setChecked(state.filter_deleted);
	m_chk_itm->setChecked(state.filter_itm_only);
}
