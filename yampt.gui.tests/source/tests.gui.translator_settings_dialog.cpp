#include "gui_test_support.hpp"
#include <catch2/catch_all.hpp>
#include "../../yampt.translator/source/dialog/settings/appearance_settings_view.hpp"
#include "../../yampt.translator/source/dialog/settings/shortcuts_settings_view.hpp"
#include "../../yampt.translator/source/dialog/settings/translator_settings_dialog.hpp"
#include <settings_store.hpp>
#include <utility/theme_enums.hpp>
#include <string>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QHeaderView>
#include <QList>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>

TEST_CASE("translator settings dialog applies theme and keeps compact shortcuts table", "[gui]")
{
	const auto store_name = QStringLiteral("yampt_gui_translator_dialog.ini");
	gui_test_support::remove_isolated_store(store_name);

	settings_store_t store(store_name);
	store.set_theme(theme_t::dark);
	store.sync();

	QTemporaryDir dictionaries_dir;
	REQUIRE(dictionaries_dir.isValid());
	const auto dictionaries_path = dictionaries_dir.path().toStdString();

	translator_settings_dialog_t dialog(store, dictionaries_path, nullptr);
	dialog.resize(600, 450);

	auto * appearance_view = dialog.findChild<translator_appearance_settings_view_t *>();
	REQUIRE(appearance_view != nullptr);

	const auto appearance_combos = appearance_view->findChildren<QComboBox *>();
	REQUIRE(appearance_combos.size() == 1);
	appearance_combos.at(0)->setCurrentIndex(static_cast<int>(theme_t::light));

	auto * button_box = dialog.findChild<QDialogButtonBox *>();
	REQUIRE(button_box != nullptr);

	auto * apply_button = button_box->button(QDialogButtonBox::Apply);
	REQUIRE(apply_button != nullptr);

	QTest::mouseClick(apply_button, Qt::LeftButton);

	REQUIRE(store.theme() == theme_t::light);

	auto * shortcuts_view = dialog.findChild<shortcuts_settings_view_t *>();
	REQUIRE(shortcuts_view != nullptr);

	auto * shortcuts_table = shortcuts_view->findChild<QTableWidget *>();
	REQUIRE(shortcuts_table != nullptr);
	REQUIRE(shortcuts_table->verticalHeader()->defaultSectionSize() == 24);

	gui_test_support::remove_isolated_store(store_name);
}
