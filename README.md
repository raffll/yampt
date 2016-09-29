# Yet Another Morrowind Plugin Translator

## Description

Simple command line tool for automatic translation from one language to another. It works on cell names, gsmt strings, object names, birthsigns, class and race descriptions, book text, faction rank names, magic and skill descriptions, dialog topic names, dialog text and script lines. In one word, on all readable in-game text. It can be used as well for making no-esp patches without creating DELE records. It can also add your native dialog topic names to the end of not converted INFO strings.

The idea was to quickly convert an infinite number of plugins with an infinite number of combined dictionaries, with one command. To eliminate inconsistencies between files.

All you need is two language version of the game (in most cases English and your native) or pre-made dictionaries.

After convertion you must recompile all scripts in TES CS (or MWEdit)!

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
Here you have one "yampt-merged.dic". This is your base dictionary.

Above command validate, sort and remove duplicates, so you can use it with only one dictionary:
```
yampt.exe --merge -d Morrowind.dic
```
## Converting esm/esp/ess
```
yampt.exe --convert -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "yampt-merged.dic"
```
### If you want to add dialog topic names to not converted INFO strings
```
yampt.exe --convert --add-dial -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "yampt-merged.dic"
```
Without this, most English plugins are not playable in your native language.

Because of limitation of Morrowind engine, INFO string can only have 512 bytes, but more is ok in game.

This can generate warnings in TES CS and records are read only. MWEdit have problem with bigger strings, so don't use this option when converting MWSE plugins.

### If you want to convert only CELL, DIAL, BNAM and SCTX records
```
yampt.exe --convert --safe -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "yampt-merged.dic"
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
yampt.exe --make-all -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "yampt-merged.dic"
```
Use for manualy translate everything, but with little help.

### Making dictionary from plugin with records that don't exist in selected dictionaries
```
yampt.exe --make-not -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "yampt-merged.dic"
```
Use for manualy translate only new records.

### Compare file with dictionary
```
yampt.exe --compare -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "yampt-merged.dic"
```
Then you can check differences in "yampt.log".

## Dictionary format

```
<h3>CELL^foreign</h3>native<hr>                              # Cell or region name
<h3>DIAL^foreign</h3>native<hr>                              # Dialog topic name
<h3>INDX^id^hardcoded key</h3>native<hr>                     # Magic or skill description
<h3>RNAM^key^number</h3>native<hr>                           # Faction rank name
<h3>DESC^id^key</h3>native<hr>                               # Birthsign, class or race description
<h3>GMST^key</h3>native<hr>                                  # GMST
<h3>FNAM^id^key</h3>native<hr>                               # Object name
<h3>INFO^dialog type^native dialog name^key</h3>native<hr>   # Dialog topic
<h3>TEXT^key</h3>native<hr>                                  # Book text
<h3>BNAM^foreign</h3>
        ^native<hr>                                          # Dialog topic script message line
<h3>SCTX^foreign</h3>
        ^native<hr>                                          # Script message line
```
- Don't forget of ^ character before native text.
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

## Useful scripts
```
REM Set path to your Wrye Mash installers folder or where you keep files

SET PATH=

del file_list.txt

for /R "%PATH%" %%f in (*.esp, *.esm, *.ess) do echo | set /p name=" "%%f" " >> file_list.txt

for /f "delims=" %%x in (file_list.txt) do yampt.exe --convert --add-dial -f %%x -d "yampt-merged.dic"

pause
```
```
REM Set path to folders where you keep native and foreign master files

SET PATH_N=
SET PATH_F=

yampt.exe --make-base -f "%PATH_N%\Morrowind.esm" "%PATH_F%\Morrowind.esm"
yampt.exe --make-base -f "%PATH_N%\Tribunal.esm" "%PATH_F%\Tribunal.esm"
yampt.exe --make-base -f "%PATH_N%\Bloodmoon.esm" "%PATH_F%\Bloodmoon.esm"

yampt.exe --merge -d "Morrowind.dic" "Tribunal.dic" "Bloodmoon.dic"

pause
```
