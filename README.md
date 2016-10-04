# Yet Another Morrowind Plugin Translator

## Description

Simple command line tool for automatic translation from one language to another. It works on cell names, gsmt strings, object names, birthsigns, class and race descriptions, book text, faction rank names, magic and skill descriptions, dialog topic names, dialog text and script lines. In one word, on all readable in-game text. It can be used as well for making no-esp patches without creating DELE records. It can also add your native dialog topic names to the end of not converted INFO strings.

Translating at least cell records is requirement to succesfully run English mod in your native Morrowind installation.

The idea was to quickly convert an infinite number of plugins with an infinite number of combined dictionaries, with one command. To eliminate inconsistencies between files. Program also have very useful options for translators.

All you need is two language version of the game (in most cases English and your native) or pre-made dictionaries.

After convertion you must recompile all scripts in TES CS (or MWEdit)! Simply select all plugins without selecting "active" and after recompilation save as new esp containing only scripts. 

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
yampt.exe --merge -d Morrowind.dic Tribunal.dic Bloodmoon.dic -o NATIVE.dic
```
Here you have one "NATIVE.dic". This is your base dictionary.

Above command validate, sort and remove duplicates, so you can use it with only one dictionary:
```
yampt.exe --merge -d Morrowind.dic
```

## Converting esm/esp/ess
```
yampt.exe --convert -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "NATIVE.dic"
```
### If you want to add dialog topic names to not converted INFO strings
```
yampt.exe --convert --add-dial -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "NATIVE.dic"
```
Without this, most English plugins with added dialogs are not playable in your native language.

Because of limitation of Morrowind engine, INFO string can only have 512 bytes, but more is ok in game.

This can generate warnings in TES CS and records are read only. MWEdit have problem with bigger strings, so don't use this option when converting MWSE plugins.

### If you want to convert only CELL, DIAL, BNAM and SCTX records
```
yampt.exe --convert --safe -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "NATIVE.dic"
```
It's useful when mod change original text in game.

## For translators

### Making dictionary with all records from plugin
```
yampt.exe --make-raw -f "C:\path\to\Morrowind\Data Files\Plugin.esp"
```
Use for manualy translate everything or create no-esp patch.

### Following commands translate dialog topic names in INFO records e.g.
```
<h3>INFO^T^skin of the pearl^8142170481561424883</h3>Some text<hr>

to

<h3>INFO^T^skóra perły^8142170481561424883</h3>Some text<hr>
```
if in "NATIVE.dic" exist 
```
<h3>DIAL^skin of the pearl</h3>skóra perły<hr>
```
So you can use this way created dictionaries combined with your "NATIVE.dic"

### Making dictionary with all records from plugin, but with CELL, DIAL, BNAM, SCTX translation
```
yampt.exe --make-all -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "NATIVE.dic"
```
Use for manualy translate everything, but with little help.

### Making dictionary from plugin with records that don't exist in selected dictionaries
```
yampt.exe --make-not -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "NATIVE.dic"
```
Use for manualy translate only new records.

### Making dictionary with only changed text

First we need second base dictionary with foreign records
```
yampt.exe --make-all -f "C:\path\to\Morrowind.esm" -d "NATIVE.dic"
yampt.exe --make-all -f "C:\path\to\Tribunal.esm" -d "NATIVE.dic"
yampt.exe --make-all -f "C:\path\to\Bloodmoon.esm" -d "NATIVE.dic"
```
Optional
```
yampt.exe --make-all -f "C:\path\to\Morrowind Patch v1.6.6_beta.esm" -d "NATIVE.dic"
```
```
./yampt --merge -d "Morrowind.ALL.dic" "Tribunal.ALL.dic" "Bloodmoon.ALL.dic" "Morrowind Patch v1.6.6_beta.ALL.dic" -o "FOREIGN.dic"
```

And now
```
yampt.exe --make-changed -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "FOREIGN.dic"
```

Commands --make-not and --make-changed are all you need to fully translate plugin.
After some translations simply type:
```
yampt.exe --convert -f "C:\path\to\Morrowind\Data Files\Plugin.esp" -d "NATIVE.dic" "Plugin.NOT.dic" "Plugin.CHANGED.dic"
```

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

or

<h3>DIAL^skin of the pearl</h3>skóra perły<hr>
<h3>INFO^T^skin of the pearl^8142170481561424883</h3>Some text<hr>
```
In first step converter translate (or not) dialog entries, and in second it can't find corresponding INFO record.

It can be a mess, because CELL, DIAL, BNAM and SCTX records don't have unique key.
