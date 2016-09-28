# Yet Another Morrowind Plugin Translator

## Description

Simple command line tool for automatic translation from one language to another. It works on cell names, gsmt strings, object names, birthsigns, class and race descriptions, book text, faction rank names, magic and skill descriptions, dialog topic names, dialog text and script lines. In one word, on all readable in-game text. It can be used as well for making no-esp patches without creating DELE records. It can also add your native dialog topic names to the end of not converted INFO strings.

The idea was to quickly convert an infinite number of plugins with an infinite number of combined dictionaries, with one command. To eliminate inconsistencies between files.

All you need is two language version of the game (in most cases English and your native) or pre-made dictionaries.

After convertion you must recompile all scripts in TES CS!

## Installation

- Unpack somewhere
- Make backup

All files are created in yampt.exe directory, and program don't check if file exist, so be careful.

## Usage
```
yampt.exe command <additional commands> -f <file list> -d <dictionary list>
```
## Making base dictionary

This step is required for plugin automatic translation.
```
yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind\Data Files\Morrowind.esm" "C:\path\to\FOREIGN\Morrowind\Data Files\Morrowind.esm"
yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind\Data Files\Tribunal.esm" "C:\path\to\FOREIGN\Morrowind\Data Files\Tribunal.esm"
yampt.exe --make-base -f "C:\path\to\NATIVE\Morrowind\Data Files\Bloodmoon.esm" "C:\path\to\FOREIGN\Morrowind\Data Files\Bloodmoon.esm"
```
Now you can merge these dictionaries into one file. Important thing is order, just like in game.
```
yampt.exe --merge -d Morrowind.dic Tribunal.dic Bloodmoon.dic
```
Here you have one Merged.dic. This is your base dictionary.

Above command validate, sort and remove duplicates, so you can use it with only one dictionary:
```
yampt.exe --merge -d Morrowind.dic
```
## Converting esm/esp/ess
```
yampt.exe --convert -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
### If you want to add dialog topic names to not converted INFO strings
```
yampt.exe --convert --add-dial -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
Without this, most English plugins are not playable in your native language.

Because of limitation of Morrowind engine, INFO string can only have 512 bytes, but more is ok in game.

This can generate warnings in TES CS and records are read only.

### If you want to convert only CELL, DIAL, BNAM and SCTX records
```
yampt.exe --convert --safe -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
It's useful when mod change original text in game.

## For translators

### Making dictionary with all records from plugin
```
yampt.exe --make-raw -f "C:\path\to\Morrowind\Data Files\Plugin.esp"
```
Use for manualy translate everything.

### Making dictionary with all records from plugin, but with CELL, DIAL, BNAM, SCTX and INFO dialog topic names translation
```
yampt.exe --make-all -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
Use for manualy translate everything, but with little help.

### Making dictionary from plugin with records that don't exist in selected dictionaries
```
yampt.exe --make-not -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
Use for manualy translate only new records.

## Options

### Add the --more-info switch if you want to INFO string could be more than 512 characters.
```
yampt.exe --convert --more-info -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
or
```
yampt.exe --merge --more-info -d "Merged.dic"
```
### Add the --log switch if you want to generate detailed log.
```
yampt.exe --convert --log -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
or
```
yampt.exe --merge --log -d "Merged.dic"
```
You can freely combine additional commands e.g.
```
yampt.exe --convert --safe --add-dial --more-info --log -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
## Dictionary format

#### Cell or region name
```
<h3>CELL^foreign</h3>native<hr>
```
#### Dialog topic name
```
<h3>DIAL^foreign</h3>native<hr>
```
#### Magic or skill description
```
<h3>INDX^id^hardcoded_key</h3>native<hr>
```
Where id is MGEF or SKIL.

#### Faction rank name
```
<h3>RNAM^key^number</h3>native<hr>
```
#### Birthsign, class or race description
```
<h3>DESC^id^key</h3>native<hr>
```
Where id is BSGN, CLAS or RACE.

#### GMST
```
<h3>GMST^key</h3>native<hr>
```
#### Object name
```
<h3>FNAM^id^key</h3>native<hr>
```
Where id is ACTI, ALCH, APPA, ARMO, BOOK, BSGN, CLAS, CLOT, CONT, CREA, DOOR, FACT, INGR, LIGH, LOCK, MISC, NPC_, PROB, RACE, REGN, REPA, SKIL, SPEL or WEAP.

#### Dialog topic
```
<h3>INFO^dialog_type^dialog_name^key</h3>native<hr>
```
Where dialog_type is T, V, G, P or J and dialog_name is native dialog topic name.

#### Book text
```
<h3>TEXT^key</h3>native<hr>
```
#### Dialog topic script message line
```
<h3>BNAM^foreign</h3>
        ^native<hr>
```
#### Script message line
```
<h3>SCTX^foreign</h3>
        ^native<hr>
```
Don't forget of ^ character before native text.

## Best way to translate
```
yampt.exe --make-all -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic"
```
```
yampt.exe --merge --log -d "Merged.dic" "Plugin.dic"
```
Then you can check differences in "yampt-merger-log" and do some changes in "Plugin.dic".
```
yampt.exe --convert --log -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "Merged.dic" "Plugin.dic"
```
In this case "Plugin.dic" have higher priority than "Merged.dic".

## Tips

- If you change something in dictionary, make sure that text editor doesn't change encoding.
- If you lose html tag dictionary won't load.
- Make sure that dictionary doesn't contains:
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

It can be a mess, because CELL, DIAL, BNAM and SCTX records don't have unique key.

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
- redesign interface
	- remove --convert-with-dial
	- remove --convert-safe
	- remove --scripts
	- remove -r
	- add --add-dial option
	- add --safe option
	- rename -a to --more-info
- add --log command (detailed converter and merger log)
- add detailed output log
- add limit of RNAM string to 32
- creator and converter now ommit FNAM "player" record

