# Yet Another Morrowind Plugin Translator

## Description

Simple command line tool for automatic translation from one language to another. It works on cell names, gsmt strings, object names, birthsigns, class and race descriptions, book text, faction rank names, magic and skill descriptions, dialog topic names, dialog text, script lines and compiled script data. In one word, on all readable in-game text. It can be used as well for making no-esp patches without creating DELE records.

Because translating at least cell records is requirement to succesfully run English mod on your native Morrowind installation. The idea is to quickly convert multiple plugins with multiple combined dictionaries, with one command, to eliminate inconsistencies between files.

Second big problem are missing hyperlinks. Program solve this by adding your native dialog topic names to the end of not converted INFO strings.

All you need is two language version of the game (in most cases English and your native) or pre-made dictionaries.

## Making base dictionary

This step is required for plugin automatic translation.
```
yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind\Data Files\Morrowind.esm" "C:\path\to\FOREIGN\Morrowind\Data Files\Morrowind.esm"
yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind\Data Files\Tribunal.esm" "C:\path\to\FOREIGN\Morrowind\Data Files\Tribunal.esm"
yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind\Data Files\Bloodmoon.esm" "C:\path\to\FOREIGN\Morrowind\Data Files\Bloodmoon.esm"
```
Now you can merge these dictionaries into one file. Important thing is order, just like in game.
```
yampt.exe --merge -d Morrowind.BASE.xml Tribunal.BASE.xml Bloodmoon.BASE.xml -o NATIVE.xml
```
Here you have one "NATIVE.xml". This is your base dictionary.

Above command validate, sort and remove duplicates, so you can use it with only one dictionary:
```
yampt.exe --merge -d Morrowind.xml
```

## Converting esm/esp/ess
```
yampt.exe --convert -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "NATIVE.xml"
```
### If you want to add dialog topic names to not converted INFO strings (not found in dictionary)
```
yampt.exe --convert -a -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "NATIVE.xml"
```
Text:
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
    <id>DIAL</id>
    <key>skin of the pearl</key>
    <val>skóra perły</val>
<record>
```
Without this, most English plugins with new dialogs are not playable in your native language.

Because of limitation of Morrowind engine, INFO string can only have 512 bytes, but more is ok in game.

This can generate warnings in TES CS and records are read only.

## For translators

### Making dictionary with all records from plugin
```
yampt.exe --make-raw -f "C:\path\to\Morrowind\Data Files\Plugin.esp"
```
Use for manualy translate everything or create no-esp patch.

### Following commands translate dialog topic names in INFO records e.g.
```
<record>
    <id>INFO</id>
    <key>T^skin of the pearl^8142170481561424883</key>
    <val>Some text</val>
<record>
```
to
```
<record>
    <id>INFO</id>
    <key>T^skóra perły^8142170481561424883</key>
    <val>Some text</val>
<record>
```
if in "NATIVE.xml" exist:
```
<record>
    <id>DIAL</id>
    <key>skin of the pearl</key>
    <val>skóra perły</val>
<record>
```
This is important because converter in first step convert DIAL record and then corresponding INFO records

### Making dictionary with all records from plugin, but with CELL, DIAL, BNAM, SCTX translation
```
yampt.exe --make-all -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "NATIVE.xml"
```
Use for manualy translate everything.

### Making dictionary from plugin with records that don't exist in selected dictionaries
```
yampt.exe --make-not -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "NATIVE.xml"
```
Use for manualy translate only new records.

### Making dictionary with only changed text

First we need second base dictionary with foreign records
```
yampt.exe --make-all -f "C:\path\to\FOREIGN\Morrowind.esm" -d "NATIVE.xml"
yampt.exe --make-all -f "C:\path\to\FOREIGN\Tribunal.esm" -d "NATIVE.xml"
yampt.exe --make-all -f "C:\path\to\FOREIGN\Bloodmoon.esm" -d "NATIVE.xml"

yampt.exe --merge -d "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml" -o "FOREIGN.xml"
```
And now
```
yampt.exe --make-changed -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "FOREIGN.xml"
```
Commands "--make-not" and "--make-changed" is all you need to fully translate plugin.
After some translations simply type:
```
yampt.exe --convert -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "NATIVE.xml" "Plugin.NOTFOUND.xml" "Plugin.CHANGED.xml"
```

## Tools

### Binary dump
```
yampt.exe --binary-dump -f "C:\path\to\Morrowind\Data Files\Plugin.esp"
```

### Script list
```
yampt.exe --scripts -f "C:\path\to\Morrowind\Data Files\Plugin.esp"
```
