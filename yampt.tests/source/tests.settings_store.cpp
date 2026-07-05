#include <catch2/catch_all.hpp>
#include <rapidcheck/catch.h>
#include <rapidcheck.h>
#include <settings_store.hpp>
#include <QCoreApplication>
#include <QFile>
#include <cmath>
#include <string>
#include <vector>

namespace {

rc::Gen<std::string> gen_printable_string()
{
	return rc::gen::exec([]()
	{
		const auto length = *rc::gen::inRange(0, 32);
		std::string result;
		result.reserve(length);
		for (int index = 0; index < length; ++index)
			result += *rc::gen::inRange(static_cast<char>(0x20), static_cast<char>(0x7E));
		return result;
	});
}

rc::Gen<std::vector<std::string>> gen_string_vector()
{
	return rc::gen::exec([]()
	{
		const auto count = *rc::gen::inRange(0, 8);
		std::vector<std::string> result;
		result.reserve(count);
		for (int index = 0; index < count; ++index)
			result.push_back(*gen_printable_string());
		return result;
	});
}

rc::Gen<theme_t> gen_theme()
{
	return rc::gen::element(theme_t::light, theme_t::dark);
}

} // namespace

TEST_CASE("settings_store_t, round-trip", "[pbt]")
{
	rc::prop(
		"Validates: Requirements 15.1",
		[]()
	{
		const auto filename = QString("yampt_pbt_settings_store_test.ini");

		{
			settings_store_t store(filename);

			const auto encoding_index = *rc::gen::inRange(-100, 100);
			store.set_encoding_index(encoding_index);

			const auto column_index = *rc::gen::inRange(0, 10);
			const auto column_width = *rc::gen::inRange(10, 2000);
			store.set_column_width(column_index, column_width);

			const auto native_language = *gen_printable_string();
			store.set_native_language(native_language);

			const auto spell_aff_path = *gen_printable_string();
			store.set_spell_aff_path(spell_aff_path);

			const auto foreign_tag = *gen_printable_string();
			store.set_foreign_tag(foreign_tag);

			const auto split_ratio = *rc::gen::inRange(0, 1000) / 1000.0f;
			store.set_split_ratio(split_ratio);

			const auto sidebar_visible = *rc::gen::arbitrary<bool>();
			store.set_sidebar_visible(sidebar_visible);

			const auto merge_fog_fix = *rc::gen::arbitrary<bool>();
			store.set_merge_fog_fix_enabled(merge_fog_fix);

			const auto workspace_roots = *gen_string_vector();
			store.set_workspace_roots(workspace_roots);

			const auto merge_order = *gen_string_vector();
			store.set_last_merge_order(merge_order);

			const auto theme = *gen_theme();
			store.set_theme(theme);

			store.sync();

			settings_store_t reader(filename);

			RC_ASSERT(reader.encoding_index() == encoding_index);
			RC_ASSERT(reader.column_width(column_index) == column_width);
			RC_ASSERT(reader.native_language() == native_language);
			RC_ASSERT(reader.spell_aff_path() == spell_aff_path);
			RC_ASSERT(reader.foreign_tag() == foreign_tag);
			RC_ASSERT(reader.sidebar_visible() == sidebar_visible);
			RC_ASSERT(reader.merge_fog_fix_enabled() == merge_fog_fix);
			RC_ASSERT(reader.workspace_roots() == workspace_roots);
			RC_ASSERT(reader.last_merge_order() == merge_order);
			RC_ASSERT(reader.theme() == theme);

			const auto read_ratio = reader.split_ratio();
			RC_ASSERT(std::abs(read_ratio - split_ratio) < 0.001f);
		}

		const auto full_path = QCoreApplication::applicationDirPath() + "/" + filename;
		QFile::remove(full_path);
	});
}
