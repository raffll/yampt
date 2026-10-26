#include "gui_test_support.hpp"
#include <catch2/catch_all.hpp>
#include "../../yampt.editor/source/dialog/settings/appearance_settings_view.hpp"
#include "../../yampt.editor/source/dialog/settings/editor_settings_dialog.hpp"
#include "../../yampt.editor/source/dialog/settings/merge_settings_view.hpp"
#include <settings_store.hpp>
#include <utility/theme_enums.hpp>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QList>
#include <QPushButton>
#include <QTableWidget>
#include <QTest>

TEST_CASE("editor settings dialog applies theme and keeps compact merge table", "[gui]")
{
	const auto store_name = QStringLiteral("yampt_gui_editor_dialog.ini");
	gui_test_support::remove_isolated_store(store_name);

	settings_store_t store(store_name);
	store.set_theme(theme_t::dark);
	store.sync();

	editor_settings_dialog_t dialog(store, nullptr);
	dialog.resize(600, 450);

	auto * appearance_view = dialog.findChild<editor_appearance_settings_view_t *>();
	REQUIRE(appearance_view != nullptr);

	const auto appearance_combos = appearance_view->findChildren<QComboBox *>();
	REQUIRE(appearance_combos.size() == 2);
	appearance_combos.at(0)->setCurrentIndex(static_cast<int>(theme_t::light));

	auto * button_box = dialog.findChild<QDialogButtonBox *>();
	REQUIRE(button_box != nullptr);

	auto * apply_button = button_box->button(QDialogButtonBox::Apply);
	REQUIRE(apply_button != nullptr);

	QTest::mouseClick(apply_button, Qt::LeftButton);

	REQUIRE(store.theme() == theme_t::light);

	auto * merge_view = dialog.findChild<merge_settings_view_t *>();
	REQUIRE(merge_view != nullptr);

	auto * exclude_table = merge_view->findChild<QTableWidget *>();
	REQUIRE(exclude_table != nullptr);
	REQUIRE(exclude_table->verticalHeader()->defaultSectionSize() == 24);

	gui_test_support::remove_isolated_store(store_name);
}
