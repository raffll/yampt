Yet Another Morrowind Plugin Translator

------------------------------
Description
------------------------------
Simple command line tool for automatic translation from one language to another. It works on cell names, gsmt strings, object names, birthsigns, class and race descriptions, book text, faction rank names, magic and skill descriptions, dialog topic names, dialog text and script lines. In one word, on all readable in-game text. It can be used for making no-esp patches as well without creating DELE records. It can also add your native dialog topic names to the end of not converted INFO strings.

All you need is two language version of the game (in most cases english and your native).

After convertion you must recompile all scripts in TES CS!

------------------------------
Making base dictionary
------------------------------
This step is required for plugin automatic translation.

yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind\Data Files\Morrowind.esm" "C:\path\to\ENGLISH\Morrowind\Data Files\Morrowind.esm"
yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind\Data Files\Tribunal.esm" "C:\path\to\ENGLISH\Morrowind\Data Files\Tribunal.esm"
yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind\Data Files\Bloodmoon.esm" "C:\path\to\ENGLISH\Morrowind\Data Files\Bloodmoon.esm"

Now you can merge these dictionaries into one file. Important thing is order, just like in game.

yampt.exe --merge -f Morrowind.dic Tribunal.dic Bloodmoon.dic

Here you have one Merged.dic. This is your base dictionary.

------------------------------
Converting esm/esp
------------------------------
Simply

yampt.exe --convert -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d Merged.dic

or

yampt.exe --convert-with-dial -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d Merged.dic

if you want to add dialog topic names to not converted INFO strings. Without this, most english plugins are not playable in your native language.

Because of limitation of Morrowind engine, INFO string can only have 512 bytes, but more is ok in game. This can generate warnings in TES CS and records are read only.

------------------------------
For translators
------------------------------
Making dictionary with all records from plugin.

yampt.exe --make-raw -f "C:\path\to\Morrowind\Data Files\Plugin.esp"

Use for manualy translate everything.

------------------------------
Making dictionary with all records from plugin, but with INFO id translation.

yampt.exe --make-all -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d Merged.dic

Use for manualy translate everything, but with little help (don't use this dictionary to convert, first translate dialog names).

e. g.

In base dictionary:
<h3>DIAL^skin of the pearl</h3>skóra perły<hr>

This option generate in your newly created dictionary:
<h3>DIAL^skin of the pearl</h3>skin of the pearl<hr>
<h3>INFO^T^skóra perły^8142170481561424883</h3>Some text<hr>

instead of

<h3>INFO^T^skin of the pearl^8142170481561424883</h3>Some text<hr>

Make sure that dictionary don't contains:

<h3>DIAL^skin of the pearl</h3>skin of the pearl<hr>
<h3>INFO^T^skóra perły^8142170481561424883</h3>Some text<hr>

or

<h3>DIAL^skin of the pearl</h3>skóra perły<hr>
<h3>INFO^T^skin of the pearl^8142170481561424883</h3>Some text<hr>

In first step converter translate (or not) dialog entries, and in second it can't find corresponding INFO record.

It can be a mess, because CELL, DIAL, BNAM and SCTX records don't have unique id.

------------------------------
Making dictionary from plugin with records they don't exist in selected dictionaries.

yampt.exe --make-not -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d Merged.dic

Use for manualy translate only new records that don't exist in original game.
This option behave like --make-all, but is safe, because if you want to convert plugin you must select base dictionary as well.

------------------------------
At the end
------------------------------
If you change something in dictionary in text editor, make sure that encoding don't change.

------------------------------
Dictionary format is:

<h3>id</h3>text<hr>

If you lose tag dictionary won't load.

------------------------------
BNAM and SCTX entries have format like this for better readability

<h3>SCTX^text</h3>
        ^text<hr>

Don't forget of ^ character

