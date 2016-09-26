# Yet Another Morrowind Plugin Translator

## Description

Simple command yampt::line tool for automatic translation from one language to another. It works on cell names, gsmt strings, object names, birthsigns, class and race descriptions, book text, faction rank names, magic and skill descriptions, dialog topic names, dialog text and script lines. In one word, on all readable in-game text. It can be used as well for making no-esp patches without creating DELE records. It can also add your native dialog topic names to the end of not converted INFO strings.

The idea was to quickly convert an infinite number of plugins with an infinite number of combined dictionaries, with one command. To eliminate inconsistencies between files.

All you need is two language version of the game (in most cases English and your native) or pre-made dictionaries.

After convertion you must recompile all scripts in TES CS!

## Installation

- Unpack somewhere
- Make backup

All files are created in yampt.exe directory, and program don't check if file exist, so be careful.

## Making base dictionary

This step is required for plugin automatic translation.
```
yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind\Data Files\Morrowind.esm" "C:\path\to\FOREIGN\Morrowind\Data Files\Morrowind.esm"
yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind\Data Files\Tribunal.esm" "C:\path\to\FOREIGN\Morrowind\Data Files\Tribunal.esm"
yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind\Data Files\Bloodmoon.esm" "C:\path\to\FOREIGN\Morrowind\Data Files\Bloodmoon.esm"
```
Now you can merge these dictionaries into one file. Important thing is order, just like in game.
```
yampt.exe --merge -f Morrowind.dic Tribunal.dic Bloodmoon.dic
```
Here you have one Merged.dic. This is your base dictionary.

## Converting esm/esp

### Simply:
```
yampt.exe --convert -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
### If you want to add dialog topic names to not converted INFO strings (without this, most English plugins are not playable in your native language):
```
yampt.exe --convert-with-dial -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
Because of limitation of Morrowind engine, INFO string can only have 512 bytes, but more is ok in game.
This can generate warnings in TES CS and records are read only.

### And if you want to convert only CELL and DIAL records (with adding dialog topic names to INFO)
```
yampt.exe --convert-safe -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
It's useful when mod change original text in game.

## For translators

### Making dictionary with all records from plugin.
```
yampt.exe --make-raw -f "C:\path\to\Morrowind\Data Files\Plugin.esp"
```
Use for manualy translate everything.

### Making dictionary with all records from plugin, but with CELL, DIAL, BNAM, SCTX and INFO id translation.
```
yampt.exe --make-all -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
Use for manualy translate everything, but with little help.

### Making dictionary from plugin with records that don't exist in selected dictionaries (with INFO id translation).
```
yampt.exe --make-not -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
Use for manualy translate only new records.

### Compare two dictionaries
```
yampt.exe --compare -d "C:\path\to\Dict_0.dic" "C:\path\to\Dict_1.dic"
```

### Write log with raw scripts text.
```
yampt.exe --scripts -f "C:\path\to\Morrowind\Data Files\Plugin.esp"
```

## Options

### Add the -a switch if you want to INFO string could be more than 512 characters.
```
yampt.exe --convert -a -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```

### Add the -r switch if you want to replace broken characters.
```
yampt.exe --make-base -r -f "C:\path\to\NATIVE\Morrowind\Data Files\Morrowind.esm" "C:\path\to\FOREIGN\Morrowind\Data Files\Morrowind.esm"
```
Polish version of Morrowind has few broken (not windows-1250) characters, but most text editors can handle this.
This option replace them with "?" character.

You can combine these options with all commands.

## At the end

### If you change something in dictionary, make sure that text editor doesn't change encoding.

### Dictionary format is:
```
<h3>id</h3>text<hr>
```
If you lose tag dictionary won't load.

### BNAM and SCTX entries have format like this for better readability
```
<h3>SCTX^text</h3>
        ^text<hr>
```
Don't forget of ^ character

### Make sure that dictionary doesn't contains:
```
<h3>DIAL^skin of the pearl</h3>skin of the pearl<hr>
<h3>INFO^T^skóra perły^8142170481561424883</h3>Some text<hr>
```
or
```
<h3>DIAL^skin of the pearl</h3>skóra perły<hr>
<h3>INFO^T^skin of the pearl^8142170481561424883</h3>Some text<hr>
```
In first step converter translate (or not) dialog entries, and in second it can't find corresponding INFO record.

It can be a mess, because CELL, DIAL, BNAM and SCTX records don't have unique id.

## Version history

0.1 alpha
- initial release

0.2 alpha
- add merger log
- add option to write raw script text log
- remove ENCH records from dictionary (they haven't got friendly names)
- add aiescortcell to script keywords
- add REPLACE_BROKEN_CHARS option
- minor bug fixes

0.3 alpha
- fix exception when try to convert non existent subrecord

0.4 alpha
- fix exception when RNAM record has a variable length
- fix limit of INFO string to 512 and FNAM to 32 (instead of 511 and 31)
- rewrite --make-all command
- add --convert-safe command
- remove yampt.cfg

0.5 alpha
- add --compare command
