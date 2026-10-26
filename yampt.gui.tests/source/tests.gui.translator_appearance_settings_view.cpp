#include "gui_test_support.hpp"
#include <catch2/catch_all.hpp>
#include "../../yampt.translator/source/dialog/settings/appearance_settings_view.hpp"
#include <settings_store.hpp>
#include <utility/theme_enums.hpp>
#include <QComboBox>
#include <QList>

TEST_CASE("translator appearance view round-trips theme", "[gui]")
{
	const auto seed_name = QStringLiteral("yampt_gui_translator_appearance_seed.ini");
	const auto sink_name = QStringLiteral("yampt_gui_translator_appearance_sink.ini");
	gui_test_support::remove_isolated_store(seed_name);
	gui_test_support::remove_isolated_store(sink_name);

	translator_appearance_settings_view_t view;
	view.resize(400, 300);

	settings_store_t store_a(seed_name);
	store_a.set_theme(theme_t::dark);
	store_a.sync();

	view.load(store_a);

	settings_store_t store_b(sink_name);
	view.save(store_b);
	store_b.sync();

	REQUIRE(store_b.theme() == theme_t::dark);

	const auto combos = view.findChildren<QComboBox *>();
	REQUIRE(combos.size() == 1);

	auto * theme_combo = combos.at(0);
	theme_combo->setCurrentIndex(static_cast<int>(theme_t::light));

	view.save(store_b);
	store_b.sync();

	REQUIRE(store_b.theme() == theme_t::light);

	gui_test_support::remove_isolated_store(seed_name);
	gui_test_support::remove_isolated_store(sink_name);
}
