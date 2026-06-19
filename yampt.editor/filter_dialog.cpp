#include "filter_dialog.hpp"

#include <QVBoxLayout>

filter_dialog_t::filter_dialog_t(const std::vector<std::string> & available_types, QWidget * parent)
    : QDialog(parent)
{
	setWindowTitle("Advanced Filter");
	setModal(true);
	resize(400, 600);

	auto * main_layout = new QVBoxLayout(this);

	grp_conflict_all_ = new QGroupBox("Conflict All", this);
	grp_conflict_all_->setCheckable(true);
	grp_conflict_all_->setChecked(false);
	auto * ca_layout = new QVBoxLayout(grp_conflict_all_);
	chk_ca_only_one_ = new QCheckBox("Only One", grp_conflict_all_);
	chk_ca_no_conflict_ = new QCheckBox("No Conflict", grp_conflict_all_);
	chk_ca_override_ = new QCheckBox("Override", grp_conflict_all_);
	chk_ca_conflict_ = new QCheckBox("Conflict", grp_conflict_all_);
	ca_layout->addWidget(chk_ca_only_one_);
	ca_layout->addWidget(chk_ca_no_conflict_);
	ca_layout->addWidget(chk_ca_override_);
	ca_layout->addWidget(chk_ca_conflict_);
	main_layout->addWidget(grp_conflict_all_);

	grp_conflict_this_ = new QGroupBox("Conflict This", this);
	grp_conflict_this_->setCheckable(true);
	grp_conflict_this_->setChecked(false);
	auto * ct_layout = new QVBoxLayout(grp_conflict_this_);
	chk_ct_master_ = new QCheckBox("Master", grp_conflict_this_);
	chk_ct_identical_ = new QCheckBox("Identical to Master", grp_conflict_this_);
	chk_ct_override_ = new QCheckBox("Override Wins", grp_conflict_this_);
	chk_ct_wins_ = new QCheckBox("Conflict Wins", grp_conflict_this_);
	chk_ct_loses_ = new QCheckBox("Conflict Loses", grp_conflict_this_);
	chk_ct_deleted_ = new QCheckBox("Deleted", grp_conflict_this_);
	ct_layout->addWidget(chk_ct_master_);
	ct_layout->addWidget(chk_ct_identical_);
	ct_layout->addWidget(chk_ct_override_);
	ct_layout->addWidget(chk_ct_wins_);
	ct_layout->addWidget(chk_ct_loses_);
	ct_layout->addWidget(chk_ct_deleted_);
	main_layout->addWidget(grp_conflict_this_);

	auto * grp_type = new QGroupBox("Record Type", this);
	auto * type_layout = new QVBoxLayout(grp_type);
	chk_by_type_ = new QCheckBox("Filter by Type", grp_type);
	lst_types_ = new QListWidget(grp_type);
	for (const auto & t : available_types)
	{
		auto * item = new QListWidgetItem(QString::fromStdString(t));
		item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
		item->setCheckState(Qt::Unchecked);
		lst_types_->addItem(item);
	}
	type_layout->addWidget(chk_by_type_);
	type_layout->addWidget(lst_types_);
	main_layout->addWidget(grp_type);

	auto * grp_id_name = new QGroupBox("Editor ID / Name", this);
	auto * id_name_layout = new QVBoxLayout(grp_id_name);
	chk_by_id_ = new QCheckBox("By Editor ID", grp_id_name);
	edt_id_ = new QLineEdit(grp_id_name);
	edt_id_->setPlaceholderText("Substring match (case-insensitive)");
	chk_by_name_ = new QCheckBox("By Display Name", grp_id_name);
	edt_name_ = new QLineEdit(grp_id_name);
	edt_name_->setPlaceholderText("Substring match (case-insensitive)");
	id_name_layout->addWidget(chk_by_id_);
	id_name_layout->addWidget(edt_id_);
	id_name_layout->addWidget(chk_by_name_);
	id_name_layout->addWidget(edt_name_);
	main_layout->addWidget(grp_id_name);

	auto * grp_special = new QGroupBox("Special", this);
	auto * special_layout = new QVBoxLayout(grp_special);
	chk_deleted_ = new QCheckBox("Deleted only", grp_special);
	chk_itm_ = new QCheckBox("ITM only", grp_special);
	special_layout->addWidget(chk_deleted_);
	special_layout->addWidget(chk_itm_);
	main_layout->addWidget(grp_special);

	auto * buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	main_layout->addWidget(buttons);

	connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

filter_dialog_t::filter_state_t filter_dialog_t::state() const
{
	filter_state_t s;

	s.filter_conflict_all = grp_conflict_all_->isChecked();
	if (s.filter_conflict_all)
	{
		if (chk_ca_only_one_->isChecked())
			s.conflict_all_set.insert(conflict_all_t::only_one);
		if (chk_ca_no_conflict_->isChecked())
			s.conflict_all_set.insert(conflict_all_t::no_conflict);
		if (chk_ca_override_->isChecked())
			s.conflict_all_set.insert(conflict_all_t::override_benign);
		if (chk_ca_conflict_->isChecked())
			s.conflict_all_set.insert(conflict_all_t::conflict);
	}

	s.filter_conflict_this = grp_conflict_this_->isChecked();
	if (s.filter_conflict_this)
	{
		if (chk_ct_master_->isChecked())
			s.conflict_this_set.insert(conflict_this_t::master);
		if (chk_ct_identical_->isChecked())
			s.conflict_this_set.insert(conflict_this_t::identical_to_master);
		if (chk_ct_override_->isChecked())
			s.conflict_this_set.insert(conflict_this_t::override_wins);
		if (chk_ct_wins_->isChecked())
			s.conflict_this_set.insert(conflict_this_t::conflict_wins);
		if (chk_ct_loses_->isChecked())
			s.conflict_this_set.insert(conflict_this_t::conflict_loses);
		if (chk_ct_deleted_->isChecked())
			s.conflict_this_set.insert(conflict_this_t::deleted);
	}

	s.filter_by_type = chk_by_type_->isChecked();
	if (s.filter_by_type)
	{
		for (int i = 0; i < lst_types_->count(); ++i)
		{
			const auto * item = lst_types_->item(i);
			if (item->checkState() == Qt::Checked)
				s.type_set.insert(item->text().toStdString());
		}
	}

	s.filter_by_id = chk_by_id_->isChecked();
	s.id_text = edt_id_->text().toStdString();

	s.filter_by_name = chk_by_name_->isChecked();
	s.name_text = edt_name_->text().toStdString();

	s.filter_deleted = chk_deleted_->isChecked();
	s.filter_itm_only = chk_itm_->isChecked();

	return s;
}

void filter_dialog_t::set_state(const filter_state_t & state)
{
	grp_conflict_all_->setChecked(state.filter_conflict_all);
	chk_ca_only_one_->setChecked(state.conflict_all_set.count(conflict_all_t::only_one) > 0);
	chk_ca_no_conflict_->setChecked(state.conflict_all_set.count(conflict_all_t::no_conflict) > 0);
	chk_ca_override_->setChecked(state.conflict_all_set.count(conflict_all_t::override_benign) > 0);
	chk_ca_conflict_->setChecked(state.conflict_all_set.count(conflict_all_t::conflict) > 0);

	grp_conflict_this_->setChecked(state.filter_conflict_this);
	chk_ct_master_->setChecked(state.conflict_this_set.count(conflict_this_t::master) > 0);
	chk_ct_identical_->setChecked(state.conflict_this_set.count(conflict_this_t::identical_to_master) > 0);
	chk_ct_override_->setChecked(state.conflict_this_set.count(conflict_this_t::override_wins) > 0);
	chk_ct_wins_->setChecked(state.conflict_this_set.count(conflict_this_t::conflict_wins) > 0);
	chk_ct_loses_->setChecked(state.conflict_this_set.count(conflict_this_t::conflict_loses) > 0);
	chk_ct_deleted_->setChecked(state.conflict_this_set.count(conflict_this_t::deleted) > 0);

	chk_by_type_->setChecked(state.filter_by_type);
	for (int i = 0; i < lst_types_->count(); ++i)
	{
		auto * item = lst_types_->item(i);
		const auto type_str = item->text().toStdString();
		item->setCheckState(state.type_set.count(type_str) > 0 ? Qt::Checked : Qt::Unchecked);
	}

	chk_by_id_->setChecked(state.filter_by_id);
	edt_id_->setText(QString::fromStdString(state.id_text));

	chk_by_name_->setChecked(state.filter_by_name);
	edt_name_->setText(QString::fromStdString(state.name_text));

	chk_deleted_->setChecked(state.filter_deleted);
	chk_itm_->setChecked(state.filter_itm_only);
}
