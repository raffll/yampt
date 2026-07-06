#include "appearance_settings_view.hpp"
#include <settings_store.hpp>
#include <theme_system.hpp>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>

appearance_settings_view_t::appearance_settings_view_t(QWidget * parent)
    : QWidget(parent)
{
	auto * layout = new QVBoxLayout(this);

	auto * label = new QLabel("Theme:", this);
	layout->addWidget(label);

	m_theme_combo = new QComboBox(this);
	m_theme_combo->addItem("Light");
	m_theme_combo->addItem("Dark");
	m_theme_combo->setToolTip("Switch between light and dark color theme");
	layout->addWidget(m_theme_combo);

	layout->addStretch();
}

void appearance_settings_view_t::load(const settings_store_t & settings)
{
	m_initial_theme = settings.theme();
	m_theme_combo->setCurrentIndex(static_cast<int>(m_initial_theme));
}

void appearance_settings_view_t::save(settings_store_t & settings) const
{
	const auto theme = static_cast<theme_t>(m_theme_combo->currentIndex());
	settings.set_theme(theme);
	theme_system_t::instance().set_theme(theme);
}

bool appearance_settings_view_t::is_modified() const
{
	const auto current_theme = static_cast<theme_t>(m_theme_combo->currentIndex());
	return current_theme != m_initial_theme;
}
