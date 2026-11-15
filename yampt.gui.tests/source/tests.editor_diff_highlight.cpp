#include <catch2/catch_all.hpp>
#include <view/editor_view.hpp>
#include <string>
#include <QChar>
#include <QString>
#include <QTextEdit>

namespace {

bool any_selection_covers(
    const QList<QTextEdit::ExtraSelection> & selections,
    const QString & expected_text,
    int expected_start)
{
	for (const auto & selection : selections)
	{
		if (selection.cursor.selectedText() == expected_text &&
		    selection.cursor.selectionStart() == expected_start)
			return true;
	}

	return false;
}

bool no_selection_has_replacement_char(const QList<QTextEdit::ExtraSelection> & selections)
{
	for (const auto & selection : selections)
	{
		if (selection.cursor.selectedText().contains(QChar(0xFFFD)))
			return false;
	}

	return true;
}

} // namespace

TEST_CASE("editor_view::highlight_adapted_diff, polish accented substitution highlights whole character", "[gui]")
{
	editor_view_t view;

	const std::string new_text = "mu\xC5\x82""a";
	const std::string adapted_from = "mu\xC5\x9B""a";

	const auto selections = view.highlight_adapted_diff(new_text, adapted_from);

	REQUIRE(!selections.isEmpty());
	REQUIRE(no_selection_has_replacement_char(selections));

	const QString s_with_acute = QString::fromUtf8("\xC5\x9B");
	REQUIRE(any_selection_covers(selections, s_with_acute, 2));
}

TEST_CASE("editor_view::highlight_adapted_diff, cyrillic substitution highlights whole character", "[gui]")
{
	editor_view_t view;

	const std::string new_text = "\xD0\x91\xD0\xB0\xD0\xBB\xD0\xBC\xD0\xB0\xD1\x80\xD0\xB0";
	const std::string adapted_from = "\xD0\x91\xD0\xB0\xD0\xBB\xD0\xBC\xD0\xBE\xD1\x80\xD0\xB0";

	const auto selections = view.highlight_adapted_diff(new_text, adapted_from);

	REQUIRE(!selections.isEmpty());
	REQUIRE(no_selection_has_replacement_char(selections));

	const QString cyrillic_o = QString::fromUtf8("\xD0\xBE");
	REQUIRE(any_selection_covers(selections, cyrillic_o, 4));
}
