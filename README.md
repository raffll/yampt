# Yet Another Morrowind Plugin Translator

## Description

Simple command line tool for automatic translation from one language to another. It works on cell names, game settings, object names, birthsigns, class and race descriptions, book text, faction rank names, magic and skill descriptions, dialog topic names, dialog text, script lines and compiled script data. In one word, on all readable in-game text.

## Features

- Convert multiple plugins with multiple combined dictionaries to eliminate inconsistencies between files.
- Add your native dialog topic names to the end of not converted INFO strings, because without missing hyperlinks mods with lots of text don't work correctly.
- Convert compiled script data, so you don't need to recompile scripts in TES CS.
- You can add suffix to converted files, if you convert them all (including master), to create different (e.g. translated) plugin tree.
- Converter doesn't change file modify time to keep original plugin sorting.
- All you need to start are two language versions of the game (in most cases English and your native) or pre-made dictionaries.

## Usage
```
yampt.exe [command] -f <plugin list> -d <dictionary list>
```

## Making base dictionary

This step is required for plugin automatic translation.
```
yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind.esm" "C:\path\to\FOREIGN\Morrowind.esm"
yampt.exe --make-base -f "C:\path\to\NATIVE\Tribunal.esm" "C:\path\to\FOREIGN\Tribunal.esm"
yampt.exe --make-base -f "C:\path\to\NATIVE\Bloodmoon.esm" "C:\path\to\FOREIGN\Bloodmoon.esm"
```
Because algorithm isn't 100% accurate some records may need manually editing:
```
<record>
    <_id>CELL</_id>
    <key>Arenim Ancestral Tomb</key>
    <val>MISSING</val>
</record>
```
to:
```
<record>
    <_id>CELL</_id>
    <key>Arenim Ancestral Tomb</key>
    <val>Arenim-Ahnengruft</val>
</record>
```
Now you can merge these dictionaries into one file. Important thing is order, just like in game.
```
yampt.exe --merge -d Morrowind.BASE.xml Tribunal.BASE.xml Bloodmoon.BASE.xml -o NATIVE.xml
```
Here you have one "NATIVE.xml". This is your base dictionary.
Merge command validate, sort and remove duplicates, so you can use it with only one dictionary:
```
yampt.exe --merge -d Morrowind.xml
```

## Converting esm/esp/omwgame/omwaddon
```
yampt.exe --convert -f "C:\path\to\Plugin.esp" -d "NATIVE.xml"
```
### If you want to add suffix to converted files

Add "-s" switch:
```
yampt.exe --convert -f "C:\path\to\Plugin.esp" -d "NATIVE.xml" -s ".SUFFIX"
```
This option creates file named "Plugin.SUFFIX.esp" and if it was dependent on "Morrowind.esm", now it is dependent on "Morrowind.SUFFIX.esm".
So you can create entire secondary tree of translated plugins.

### If you want to add dialog topic names to not converted INFO strings

Add "-a" switch:
```
yampt.exe --convert -a -f "C:\path\to\Plugin.esp" -d "NATIVE.xml"
```
Then text:
```
Some text skin of the pearl some text.
```
Will be converted to:
```
Some text skin of the pearl some text. [skóra perły]
```
if in "NATIVE.xml" exist:
```
<record>
    <_id>DIAL</_id>
    <key>skin of the pearl</key>
    <val>skóra perły</val>
<record>
```
Without this, most English plugins with new dialogs are not playable in your native language.

#### Warning!

This can generate warnings in TES CS and those records are read only, but it is ok in game. You can ignore them.

## For translators

### Making dictionary with all records from plugin
```
yampt.exe --make-raw -f "C:\path\to\Plugin.esp"
```
Use for manualy translate everything or create no-esp patch.

### Following commands translate dialog topic names in INFO keys 
e.g.:
```
<record>
    <_id>INFO</_id>
    <key>T^skin of the pearl^8142170481561424883</key>
    <val>Some text</val>
<record>
```
to:
```
<record>
    <_id>INFO</_id>
    <key>T^skóra perły^8142170481561424883</key>
    <val>Some text</val>
<record>
```
if in "NATIVE.xml" exist:
```
<record>
    <_id>DIAL</_id>
    <key>skin of the pearl</key>
    <val>skóra perły</val>
<record>
```
This is important because converter in first step convert DIAL record and then corresponding INFO records.
It could be a mess because CELL, DIAL, BNAM, SCTX records don't have unique keys.
With help of your base dictionary, you can create plugin dictionary with these records already translated.

### Making dictionary with all records from plugin, but with CELL, DIAL, BNAM, SCTX translation
```
yampt.exe --make-all -f "C:\path\to\Plugin.esp" -d "NATIVE.xml"
```
Use for manualy translate everything with little help.

### Making dictionary from plugin with records that don't exist in selected dictionaries
```
yampt.exe --make-not -f "C:\path\to\Plugin.esp" -d "NATIVE.xml"
```
Use for manualy translate only new records (added in plugin).

### Making dictionary with only changed text

First we need second base dictionary with your native keys, but foreign values in INFO records.
```
yampt.exe --make-all -f "C:\path\to\FOREIGN\Morrowind.esm" -d "NATIVE.xml"
yampt.exe --make-all -f "C:\path\to\FOREIGN\Tribunal.esm" -d "NATIVE.xml"
yampt.exe --make-all -f "C:\path\to\FOREIGN\Bloodmoon.esm" -d "NATIVE.xml"

yampt.exe --merge -d "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml" -o "FOR_FIND_CHANGED_ONLY.xml"
```
And now:
```
yampt.exe --make-changed -f "C:\path\to\Plugin.esp" -d "FOR_FIND_CHANGED_ONLY.xml"
```

### Finally

Commands "--make-not" and "--make-changed" is all you need to fully translate plugin and after some translations simply type:
```
yampt.exe --convert -f "C:\path\to\Plugin.esp" -d "NATIVE.xml" "Plugin.NOTFOUND.xml" "Plugin.CHANGED.xml"
```

## Tools

### Binary dump
```
yampt.exe --binary-dump -f "C:\path\to\Plugin.esp"
```

### Script list
```
yampt.exe --script-list -f "C:\path\to\Plugin.esp"
```
