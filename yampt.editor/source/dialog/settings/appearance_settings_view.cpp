#include "appearance_settings_view.hpp"
#include <utility/theme_enums.hpp>
#include <settings_store.hpp>
#include <theme_system.hpp>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>

appearance_settings_view_t::appearance_settings_view_t(QWidget * parent)
    : QWidget(parent)
    , m_initial_theme(theme_t::light)
{
	auto * layout = new QVBoxLayout(this);

	auto * label = new QLabel("Theme:", this);
	layout->addWidget(label);

	m_theme_combo = new QComboBox(this);
	m_theme_combo->addItem("Light");
	m_theme_combo->addItem("Dark");
	m_theme_combo->setToolTip("Switch between light and dark color theme");
	layout->addWidget(m_theme_combo);

	auto * codepage_label = new QLabel("Text Codepage:", this);
	layout->addWidget(codepage_label);

	m_codepage_combo = new QComboBox(this);
	m_codepage_combo->addItem("Windows-1250 (Polish/Central European)", 1250);
	m_codepage_combo->addItem("Windows-1251 (Russian/Cyrillic)", 1251);
	m_codepage_combo->addItem("Windows-1252 (English/Western)", 1252);
	m_codepage_combo->setToolTip("Codepage used for displaying plugin text");
	layout->addWidget(m_codepage_combo);

	layout->addStretch();
}

void appearance_settings_view_t::load(const settings_store_t & settings)
{
	m_initial_theme = settings.theme();
	m_theme_combo->setCurrentIndex(static_cast<int>(m_initial_theme));

	m_initial_codepage = settings.display_codepage();
	const int codepage_idx = m_codepage_combo->findData(m_initial_codepage);
	m_codepage_combo->setCurrentIndex(codepage_idx >= 0 ? codepage_idx : 0);
}

void appearance_settings_view_t::save(settings_store_t & settings) const
{
	const auto theme = static_cast<theme_t>(m_theme_combo->currentIndex());
	settings.set_theme(theme);
	theme_system_t::instance().set_theme(theme);

	const int codepage = m_codepage_combo->currentData().toInt();
	settings.set_display_codepage(codepage);
}

bool appearance_settings_view_t::is_modified() const
{
	const bool theme_changed = static_cast<theme_t>(m_theme_combo->currentIndex()) != m_initial_theme;
	const bool codepage_changed = m_codepage_combo->currentData().toInt() != m_initial_codepage;
	return theme_changed || codepage_changed;
}
