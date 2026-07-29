#include "view/lua_conflicts_view.hpp"
#include "model/lua_nav_model.hpp"
#include "model/lua_registrations_model.hpp"
#include "view/lua_detail_view.hpp"
#include "view/lua_filter_view.hpp"
#include <QItemSelectionModel>
#include <QLabel>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>

lua_conflicts_view_t::lua_conflicts_view_t(QWidget * parent)
    : QWidget(parent)
{
	setup_layout();

	connect(m_filter_view, &lua_filter_view_t::filters_changed, this, &lua_conflicts_view_t::apply_filter);
}

void lua_conflicts_view_t::set_scan_result(const lua_scan_result_t & result)
{
	m_detail_view->clear();

	if (result.registrations.empty())
	{
		m_splitter->hide();
		m_empty_label->setText(tr("No .omwscripts files found."));
		m_empty_label->show();
		return;
	}

	m_empty_label->hide();
	m_splitter->show();

	if (!result.conflicts.empty())
	{
		show_conflicts_mode(result);
		return;
	}

	show_registrations_mode(result);
}

void lua_conflicts_view_t::show_conflicts_mode(const lua_scan_result_t & result)
{
	m_in_conflicts_mode = true;
	m_filter_view->show();
	m_tree_view->setModel(m_nav_model);
	m_nav_model->set_conflicts(result.conflicts);
	m_filter_view->set_conflicts(result.conflicts);

	disconnect(m_tree_view->selectionModel(), nullptr, this, nullptr);
	connect(
	    m_tree_view->selectionModel(),
	    &QItemSelectionModel::currentChanged,
	    this,
	    [this] { on_conflict_selection_changed(); });
}

void lua_conflicts_view_t::show_registrations_mode(const lua_scan_result_t & result)
{
	m_in_conflicts_mode = false;
	m_filter_view->hide();
	m_tree_view->setModel(m_registrations_model);
	m_registrations_model->set_registrations(result.registrations);
	m_tree_view->expandAll();

	disconnect(m_tree_view->selectionModel(), nullptr, this, nullptr);
	connect(
	    m_tree_view->selectionModel(),
	    &QItemSelectionModel::currentChanged,
	    this,
	    [this] { on_registration_selection_changed(); });
}

void lua_conflicts_view_t::setup_layout()
{
	auto * layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_empty_label = new QLabel(tr("No .omwscripts files found."), this);
	m_empty_label->setAlignment(Qt::AlignCenter);
	layout->addWidget(m_empty_label);

	m_splitter = new QSplitter(Qt::Horizontal, this);

	m_left_widget = new QWidget(m_splitter);
	auto * left_layout = new QVBoxLayout(m_left_widget);
	left_layout->setContentsMargins(0, 0, 0, 0);

	m_nav_model = new lua_nav_model_t(this);
	m_registrations_model = new lua_registrations_model_t(this);

	m_tree_view = new QTreeView(m_left_widget);
	m_tree_view->setModel(m_nav_model);
	m_tree_view->setHeaderHidden(true);
	left_layout->addWidget(m_tree_view, 1);

	m_filter_view = new lua_filter_view_t(m_left_widget);
	left_layout->addWidget(m_filter_view);

	m_detail_view = new lua_detail_view_t(m_splitter);

	m_splitter->addWidget(m_left_widget);
	m_splitter->addWidget(m_detail_view);
	m_splitter->setStretchFactor(0, 1);
	m_splitter->setStretchFactor(1, 2);

	layout->addWidget(m_splitter);
	m_splitter->hide();
}

void lua_conflicts_view_t::apply_filter()
{
	if (!m_in_conflicts_mode)
		return;

	const auto severities = m_filter_view->enabled_severities();
	const auto interfaces = m_filter_view->enabled_interfaces();

	lua_nav_model_t::severity_set_t severity_set;
	severity_set.blocking = severities.count(conflict_severity_t::blocking) > 0;
	severity_set.mutating = severities.count(conflict_severity_t::mutating) > 0;
	severity_set.overlapping = severities.count(conflict_severity_t::overlapping) > 0;

	m_nav_model->set_filter(severity_set, interfaces);
}

void lua_conflicts_view_t::on_conflict_selection_changed()
{
	const auto current = m_tree_view->currentIndex();
	const auto * conflict = m_nav_model->conflict_at(current);

	if (conflict == nullptr)
	{
		m_detail_view->clear();
		return;
	}

	m_detail_view->show_conflict(*conflict);
}

void lua_conflicts_view_t::on_registration_selection_changed()
{
	const auto current = m_tree_view->currentIndex();
	const auto * registration = m_registrations_model->registration_at(current);

	if (registration == nullptr)
	{
		m_detail_view->clear();
		return;
	}

	m_detail_view->show_registration(*registration);
}
