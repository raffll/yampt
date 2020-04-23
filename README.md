# **Yet Another Morrowind Plugin Translator**

### Description

Simple command line tool for automatic translation from one language to another. It works on cell names, game settings, object names, birthsigns, class and race descriptions, book text, faction rank names, magic and skill descriptions, dialog topic names, dialog text, script lines and compiled script data. In one word, on all readable in-game text.

### Features

- Convert multiple plugins with multiple combined dictionaries to eliminate inconsistencies between files.
- Add your native dialog topic names to the end of not converted dialogs, because without missing hyperlinks mods with lots of text don't work correctly.
- Convert compiled script data, so you don't need to recompile scripts in TES CS.
- Converter doesn't change file modification time to keep original plugin sorting.
- All you need to start are two language versions of the game (in most cases English and your native) or pre-made dictionaries.

## Usage

### Dictionaries

Dictionaries are stored in pseudo xml files to keep original text intact. They are divided into several parts:

CELL - cell names
DIAL - dialog topics
BNAM - script line belonging to dialog (INFO string)
SCPT - script line
GMST - game settings
FNAM - object names (weapons, armors, book names, etc.)
DESC - birthsigns, class and race descriptions
TEXT - book content
RNAM - faction ranks
INDX - magic and skill descriptions
INFO - dialogs

### Making base dictionary

This step is required for plugin automatic translation.

    yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind.esm" "C:\path\to\FOREIGN\Morrowind.esm"
    yampt.exe --make-base -f "C:\path\to\NATIVE\Tribunal.esm" "C:\path\to\FOREIGN\Tribunal.esm"
    yampt.exe --make-base -f "C:\path\to\NATIVE\Bloodmoon.esm" "C:\path\to\FOREIGN\Bloodmoon.esm"

Now you can merge these dictionaries into one file. Important thing is order, just like in game.

    yampt.exe --merge -d "Morrowind.BASE.xml" "Tribunal.BASE.xml" "Bloodmoon.BASE.xml" -o "NATIVE.xml"

Here you have one *NATIVE.xml*. This is your base dictionary. Merge command validate, sort and remove duplicates, so you can use it with only one dictionary:

    yampt.exe --merge -d "Morrowind.xml"

Because algorithm isn't 100% accurate some records may need manually editing:

    <record>
        <_id>CELL</_id>
        <key>Arenim Ancestral Tomb</key>
        <val>MISSING</val>
    </record>

to:

    <record>
        <_id>CELL</_id>
        <key>Arenim Ancestral Tomb</key>
        <val>Arenim-Ahnengruft</val>
    </record>

### Converting esm/esp/omwgame/omwaddon

    yampt.exe --convert -f "C:\path\to\Plugin.esp" -d "NATIVE.xml"

### If you want to add dialog topic names to not converted INFO strings

Add *--add-hyperlinks* switch:

    yampt.exe --convert --add-hyperlinks -f "C:\path\to\Plugin.esp" -d "NATIVE.xml"

Then text:

    Some text skin of the pearl some text.

Will be converted to:

    Some text skin of the pearl some text. [skóra perły]

if in *NATIVE.xml* exist:

    <record>
        <_id>DIAL</_id>
        <key>skin of the pearl</key>
        <val>skóra perły</val>
    <record>

Without this, most English plugins with new dialogs are not playable in your native language. This switch also works with *--make-not* and *--make-changed* commands.

### Warning!

If INFO text exceeds 512 characters, this could generate warnings in TES CS and those records will be read only, but it is ok in game. You can ignore them.

## For translators

### Making dictionary with all records from plugin

    yampt.exe --make-raw -f "C:\path\to\Plugin.esp"

Use for manually translate everything or create no-esp patch.

### Making dictionary with all records from plugin, but with CELL, DIAL, BNAM, SCTX translation

    yampt.exe --make-all -f "C:\path\to\Plugin.esp" -d "NATIVE.xml"

Use for manually translate everything with little help.

### Making dictionary from plugin with records that don't exist in selected dictionaries

    yampt.exe --make-not -f "C:\path\to\Plugin.esp" -d "NATIVE.xml"

Use for manually translate only new records (added in plugin).

### Making dictionary with only changed text

First we need second base dictionary with your native keys, but foreign values in INFO records.

    yampt.exe --make-all -f "C:\path\to\FOREIGN\Morrowind.esm" -d "NATIVE.xml"
    yampt.exe --make-all -f "C:\path\to\FOREIGN\Tribunal.esm" -d "NATIVE.xml"
    yampt.exe --make-all -f "C:\path\to\FOREIGN\Bloodmoon.esm" -d "NATIVE.xml"

    yampt.exe --merge -d "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml" -o "NATIVE_for_find_changed_only.xml"

And now:

    yampt.exe --make-changed -f "C:\path\to\Plugin.esp" -d "NATIVE_for_find_changed_only.xml"

### Translate dialog topic names in INFO keys

Command *--make-all*, *--make-not* and *--make-changed* automatically convert dialog topic names in INFO keys

e.g.:

    <record>
        <_id>INFO</_id>
        <key>T^skin of the pearl^8142170481561424883</key>
        <val>Some text</val>
    <record>

to:

    <record>
        <_id>INFO</_id>
        <key>T^skóra perły^8142170481561424883</key>
        <val>Some text</val>
    <record>

if in NATIVE.xml exist:

    <record>
        <_id>DIAL</_id>
        <key>skin of the pearl</key>
        <val>skóra perły</val>
    <record>

This is important because converter in first step convert DIAL record and then corresponding INFO records. It could be a mess because CELL, DIAL, BNAM, SCTX records don't have unique keys. With help of your base dictionary, you can create plugin dictionary with these records already translated.

### Finally

Commands *--make-not* and *--make-changed* is all you need to fully translate plugin and after some translations simply type:

    yampt.exe --convert -f "C:\path\to\Plugin.esp" -d "NATIVE.xml" "Plugin.NOTFOUND.xml" "Plugin.CHANGED.xml"

## Other

### Dump plugin content

    yampt.exe --binary-dump -f "C:\path\to\Plugin.esp"

### Create list of scripts

    yampt.exe --script-list -f "C:\path\to\Plugin.esp"

### If you want to add suffix to converted files

Add --suffix switch:

    yampt.exe --convert -f "C:\path\to\Plugin.esp" -d "NATIVE.xml" -s ".SUFFIX"

This option creates file named "Plugin.SUFFIX.esp" and if it was dependent on "Morrowind.esm", now it is dependent on "Morrowind.SUFFIX.esm". So you can create entire secondary tree of translated plugins.

## Easy start!

### Warning!
Make sure *yampt.exe* and all *bat* scripts are in the same directory and this directory doesn't contain any esm, esp, or xml files!

### Creating base dictionaries

In file *yampt-make-base-template.bat* change lines:

    set NATIVE=
    set FOREIGN=

to path where you keep native and foreign master files (e.g. Morrowind.esm, Tribunal.esm, Bloodmoon.esm)

    set NATIVE=C:\path\to\native
    set FOREIGN=C:\path\to\foreign

Then double-click on script. This creates your base dictionaries in folder *dict\_base*.

### Converting multiple files at once

You can copy all original foreign plugins to folder *input* and double-click on *yampt-convert-template.bat*. This option creates converted files in folder *output*.

### Creating user dictionaries

When you run *yampt-make-user-template.cmd*, script creates *dict\_new* folder contains:

- *CHANGED.xml* dictionaries with changed text in comparison with original, previously created *NATIVE\_for\_find\_changed\_only.xml*.
- *NOTFOUND.xml* dictionaries with text not found in any dictionary, e.g. new text added by plugin.

This folder is refreshed every time you run the script. You can now translate this files and copy them to *dict\_user* folder. Dictionaries in this folder will be combined with base dictionary during next convertion.
