#include <catch2/catch_all.hpp>
#include <theme_system.hpp>
#include <utility/color_palette.hpp>
#include <utility/theme_enums.hpp>
#include <io/app_settings.hpp>
#include <QCoreApplication>
#include <cmath>
#include <cstdlib>

TEST_CASE("theme_system_t::get_color, all named colors are valid", "[u]")
{
	const auto theme = GENERATE(theme_t::light, theme_t::dark);
	const auto index = GENERATE(range(0, static_cast<int>(color_name_t::color_name_count)));
	const auto name = static_cast<color_name_t>(index);

	const auto color = theme_system_t::instance().get_color(name, theme);

	REQUIRE(color.alpha() > 0);
	REQUIRE((color.red() > 0 || color.green() > 0 || color.blue() > 0));
}

TEST_CASE("theme_system_t::get_status_color, hue preserved", "[u]")
{
	const auto status_count = static_cast<int>(status_t::error) + 1;
	const auto index = GENERATE_COPY(range(0, status_count));
	const auto status = static_cast<status_t>(index);

	const auto light_color = theme_system_t::instance().get_status_color(status, theme_t::light);
	const auto dark_color = theme_system_t::instance().get_status_color(status, theme_t::dark);

	const int light_hue = light_color.hslHue();
	const int dark_hue = dark_color.hslHue();

	if (light_hue < 0 || dark_hue < 0)
		return;

	int hue_diff = std::abs(light_hue - dark_hue);
	if (hue_diff > 180)
		hue_diff = 360 - hue_diff;

	REQUIRE(hue_diff <= 15);
}

namespace
{

double relative_luminance(const QColor & color)
{
	auto linearize = [](double channel) {
		return (channel <= 0.03928) ? channel / 12.92 : std::pow((channel + 0.055) / 1.055, 2.4);
	};
	double r = linearize(color.redF());
	double g = linearize(color.greenF());
	double b = linearize(color.blueF());
	return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

double contrast_ratio(const QColor & foreground, const QColor & background)
{
	double lum_fg = relative_luminance(foreground);
	double lum_bg = relative_luminance(background);
	double lighter = std::max(lum_fg, lum_bg);
	double darker = std::min(lum_fg, lum_bg);
	return (lighter + 0.05) / (darker + 0.05);
}

}

TEST_CASE("theme_system_t::get_color, dark foreground contrast", "[u]")
{
	const auto dark_background = theme_system_t::instance().get_color(
		color_name_t::editor_background, theme_t::dark);

	const auto foreground_names = {
		color_name_t::window_text,
		color_name_t::editor_text,
		color_name_t::syntax_function,
		color_name_t::syntax_comment,
		color_name_t::syntax_string,
		color_name_t::syntax_html_tag,
		color_name_t::syntax_hyperlink,
		color_name_t::syntax_misspelled,
		color_name_t::status_translated,
		color_name_t::status_untranslated,
		color_name_t::status_missing,
		color_name_t::status_duplicate,
		color_name_t::status_mismatch,
		color_name_t::status_error,
		color_name_t::status_reused,
		color_name_t::status_adapted,
		color_name_t::status_changed,
		color_name_t::status_outdated,
		color_name_t::status_in_progress,
		color_name_t::status_model,
		color_name_t::status_propagated,
		color_name_t::status_heuristic,
		color_name_t::status_to_verify,
		color_name_t::status_ambiguous,
		color_name_t::conflict_this_master,
		color_name_t::conflict_this_identical,
		color_name_t::conflict_this_override_wins,
		color_name_t::conflict_this_conflict_wins,
		color_name_t::conflict_this_conflict_loses,
		color_name_t::conflict_this_deleted,
	};

	for (const auto name : foreground_names)
	{
		const auto color = theme_system_t::instance().get_color(name, theme_t::dark);
		REQUIRE(contrast_ratio(color, dark_background) >= 4.5);
	}
}

TEST_CASE("theme_system_t::get_color, dark highlights semi-transparent", "[u]")
{
	const auto highlight_names = {
		color_name_t::diff_added_background,
		color_name_t::diff_removed_background,
		color_name_t::diff_changed_background,
		color_name_t::annotation_dial_topic,
		color_name_t::annotation_glossary_term,
	};

	for (const auto name : highlight_names)
	{
		const auto color = theme_system_t::instance().get_color(name, theme_t::dark);
		REQUIRE(color.alpha() < 180);
	}
}

TEST_CASE("theme_system_t::conflict_all_background, visible", "[u]")
{
	const auto theme = GENERATE(theme_t::light, theme_t::dark);
	theme_system_t::instance().set_theme(theme);

	const auto base_bg = theme_system_t::instance().get_color(color_name_t::window_background);

	const auto conflict_values = { conflict_all_t::no_conflict, conflict_all_t::override_benign, conflict_all_t::conflict };

	for (const auto value : conflict_values)
	{
		const auto bg = theme_system_t::instance().conflict_all_background(value);
		REQUIRE(contrast_ratio(bg, base_bg) >= 1.5);
	}
}

TEST_CASE("theme_system_t, conflict foreground/background contrast", "[u]")
{
	const auto theme = GENERATE(theme_t::light, theme_t::dark);
	theme_system_t::instance().set_theme(theme);

	const auto conflict_all_values = { conflict_all_t::no_conflict, conflict_all_t::override_benign, conflict_all_t::conflict };
	const auto conflict_this_values = { conflict_this_t::master, conflict_this_t::identical_to_master, conflict_this_t::override_wins, conflict_this_t::conflict_wins, conflict_this_t::conflict_loses, conflict_this_t::deleted };

	for (const auto ca : conflict_all_values)
	{
		const auto bg = theme_system_t::instance().conflict_all_background(ca);
		for (const auto ct : conflict_this_values)
		{
			const auto fg = theme_system_t::instance().conflict_this_foreground(ct);
			REQUIRE(contrast_ratio(fg, bg) >= 3.0);
		}
	}
}

TEST_CASE("theme_system_t, dark conflict backgrounds are darker", "[u]")
{
	theme_system_t::instance().set_theme(theme_t::dark);

	const auto conflict_values = { conflict_all_t::no_conflict, conflict_all_t::override_benign, conflict_all_t::conflict };

	for (const auto value : conflict_values)
	{
		const auto bg = theme_system_t::instance().conflict_all_background(value);

		color_name_t raw_name = color_name_t::conflict_all_no_conflict_raw;
		if (value == conflict_all_t::override_benign)
			raw_name = color_name_t::conflict_all_override_benign_raw;
		else if (value == conflict_all_t::conflict)
			raw_name = color_name_t::conflict_all_conflict_raw;

		const auto raw_color = theme_system_t::instance().get_color(raw_name, theme_t::dark);

		REQUIRE(bg.lightnessF() <= raw_color.lightnessF());
	}

	theme_system_t::instance().set_theme(theme_t::light);
}

TEST_CASE("app_settings_t::theme, round-trip", "[u]")
{
	app_settings_t settings("test_theme_settings.ini");
	settings.set_theme(theme_t::dark);
	REQUIRE(settings.theme() == theme_t::dark);
	settings.set_theme(theme_t::light);
	REQUIRE(settings.theme() == theme_t::light);
}

TEST_CASE("app_settings_t::theme, default is light", "[u]")
{
	app_settings_t settings("test_theme_empty.ini");
	REQUIRE(settings.theme() == theme_t::light);
}

TEST_CASE("app_settings_t::theme, invalid defaults to light", "[u]")
{
	app_settings_t settings("test_theme_invalid.ini");
	QSettings raw(QCoreApplication::applicationDirPath() + "/test_theme_invalid.ini", QSettings::IniFormat);
	raw.setValue("Appearance/Theme", "garbage");
	raw.sync();

	app_settings_t settings2("test_theme_invalid.ini");
	REQUIRE(settings2.theme() == theme_t::light);
}

TEST_CASE("theme_system_t::get_color, light backward compat", "[u]")
{
	auto check = [](color_name_t name, int r, int g, int b, int a = 255) {
		const auto color = theme_system_t::instance().get_color(name, theme_t::light);
		REQUIRE(color.red() == r);
		REQUIRE(color.green() == g);
		REQUIRE(color.blue() == b);
		REQUIRE(color.alpha() == a);
	};

	check(color_name_t::status_translated, 128, 230, 128);
	check(color_name_t::status_untranslated, 166, 166, 166);
	check(color_name_t::status_missing, 242, 140, 89);
	check(color_name_t::status_duplicate, 242, 230, 102);
	check(color_name_t::status_mismatch, 230, 115, 140);
	check(color_name_t::status_error, 242, 102, 102);
	check(color_name_t::status_reused, 128, 217, 179);
	check(color_name_t::status_adapted, 179, 140, 217);
	check(color_name_t::status_changed, 242, 179, 102);
	check(color_name_t::status_outdated, 220, 140, 80);
	check(color_name_t::status_in_progress, 102, 153, 242);
	check(color_name_t::status_model, 100, 180, 220);
	check(color_name_t::status_propagated, 180, 230, 230);
	check(color_name_t::status_heuristic, 100, 180, 160);
	check(color_name_t::status_to_verify, 180, 200, 180);
	check(color_name_t::status_ambiguous, 230, 180, 60);
	check(color_name_t::syntax_function, 100, 180, 255);
	check(color_name_t::syntax_comment, 128, 128, 128);
	check(color_name_t::syntax_string, 200, 150, 50);
	check(color_name_t::syntax_html_tag, 100, 0, 20);
	check(color_name_t::syntax_forbidden_background, 255, 200, 180);
	check(color_name_t::syntax_hyperlink, 70, 130, 220);
	check(color_name_t::syntax_misspelled, 220, 50, 50);
	check(color_name_t::diff_changed_background, 255, 200, 130);
	check(color_name_t::annotation_dial_topic, 70, 130, 200, 60);
	check(color_name_t::annotation_glossary_term, 70, 180, 70, 60);
	check(color_name_t::conflict_all_no_conflict_raw, 0, 255, 0);
	check(color_name_t::conflict_all_override_benign_raw, 255, 255, 0);
	check(color_name_t::conflict_all_conflict_raw, 255, 0, 0);
	check(color_name_t::conflict_this_master, 128, 0, 128);
	check(color_name_t::conflict_this_identical, 128, 128, 128);
	check(color_name_t::conflict_this_override_wins, 0, 128, 0);
	check(color_name_t::conflict_this_conflict_wins, 255, 128, 64);
	check(color_name_t::conflict_this_conflict_loses, 255, 0, 0);
	check(color_name_t::conflict_this_deleted, 100, 100, 100);
}
