# **Yet Another Morrowind Plugin Translator**

### Description

Simple command line tool for automatic translation from one language to another. It works on cell names, game settings, object names, birthsigns, class and race descriptions, book text, faction rank names, magic and skill descriptions, dialog topic names, dialog text, script lines and compiled script data. In one word, on all readable in-game text.

### Features

- Translate multiple plugins at once with multiple combined dictionaries to eliminate inconsistencies between files.
- Add your native dialog topic names to the end of not translated dialogs, because without missing hyperlinks mods with lots of text don't work correctly.
- Convert compiled script data, so you don't need to recompile scripts in TES CS.
- Doesn't change file modification time to keep original plugin sorting.
- All you need to start are two language versions of the game or pre-made dictionaries.

---
## Easy start!
**All examples below are for Polish version of the game**

1. Make sure **yampt.exe** and all **.bat scripts** are in the same directory and this directory doesn't contain any esm, esp, or xml files!
2. Pick one of the language e.g. ENtoDE means the script converts from English to German.
3. Copy all original foreign plugins you want to translate to folder **input**.

### Creating user dictionaries

1. Run **yampt-make-user-ENtoPL.cmd**, script creates two types of dictionary in **dict\_new** folder:
    - **.CHANGED.xml** dictionaries with changed text in comparison with original esm content, e.g. corrected typo in a patch.
    - **.NOTFOUND.xml** dictionaries with text not found in any dictionary, e.g. new text added by plugin.
2. Folder **dict\_new** is wipe out every time you run the script, so copy this files into safe place.
3. Translate this files in text editor:
    - Make sure that you use correct encoding, e.g:
        - windows-1252 for English, French, German
        - windows-1251 for Russian
        - windows-1250 for Polish
    - Replace only text between **<val></val>** tags
4. Copy translated files to **dict\_user** folder. Dictionaries in this folder will be combined with base dictionary during convertion.

### Converting multiple files at once

1. Run **yampt-convert-ENtoPL.bat**. This option creates converted files in folder **output**.
2. Read **yampt.log** and check if there are any **Error** in script parser. **If is, you have to compile scripts in CS.**

### Creating base dictionaries

Converter comes with several pre made dictionaries, but you can easly create your own:

1. In file **yampt-make-base-XXtoXX.bat** change this lines to paths where you keep native and foreign master files (e.g. Morrowind.esm, Tribunal.esm, Bloodmoon.esm).
```
set NATIVE=
set FOREIGN=
set NAME=XXtoXX
```
to
```
set NATIVE=C:\path\to\Polish
set FOREIGN=C:\path\to\English
set NAME=ENtoPL
```
2. Run the script. This creates your base dictionaries in **dict\_base** folder.
3. Edit **yampt-make-user-XXtoXX.cmd** and **yampt-convert-XXtoXX.bat** to match your new base dictionary:
```
set DICT_N=XXtoXX.xml
set DICT_F=XXtoXX_H.xml
```
to
```
set DICT_N=ENtoPL.xml
set DICT_F=ENtoPL_H.xml
```
4. Open dictionary and find any **MISSING** keywords. **You have to manually correct them.**

---
## Dictionaries

Dictionaries are stored in pseudo xml files to keep original text intact. They are divided into several parts:

- CELL - cell names
- DIAL - dialog topics
- BNAM - script line belonging to dialog (INFO string)
- SCPT - script line
- GMST - game settings
- FNAM - object names (weapons, armors, book names, etc.)
- DESC - birthsigns, class and race descriptions
- TEXT - book content
- RNAM - faction ranks
- INDX - magic and skill descriptions
- INFO - dialogs

---
## Detailed usage

### Making base dictionary
Making base dictionary works best when you have two same files but localized differently:
```
yampt.exe --make-base -f <path to native plugin> <path to foreign plugin>
```
e.g.
```
yampt.exe --make-base -f "C:\path\to\PL\Morrowind.esm" "C:\path\to\EN\Morrowind.esm"
yampt.exe --make-base -f "C:\path\to\PL\Tribunal.esm" "C:\path\to\EN\Tribunal.esm"
yampt.exe --make-base -f "C:\path\to\PL\Bloodmoon.esm" "C:\path\to\EN\Bloodmoon.esm"
```
This option creates xml file with **.BASE.xml** extension.

### Merge dictionaries
Now you can merge these dictionaries into one file. Important thing is order. When in couple dictionaries exist record with the same key, only last one from last dictionary will be used.
```
yampt.exe --merge -d <list of dictionaries> -o <output dictionary name>
```
e.g.
```
yampt.exe --merge -d "Morrowind.BASE.xml" "Tribunal.BASE.xml" "Bloodmoon.BASE.xml" -o "ENtoPL.xml"
```
The output of this command is **ENtoPL.xml**. This is your base dictionary. Merge command also validate, sort and remove duplicates.

### Converting esm/esp/omwgame/omwaddon
Simply replace all text in plugin
```
yampt.exe --convert -f <list of plugins> -d <list of dictionaries>
```
e.g.
```
yampt.exe --convert -f "C:\path\to\Plugin.esp" -d "ENtoPL.xml"
```
If you want to add hyperlinks to not converted dialogs (not found in any dictionary) add **--add-hyperlinks** switch:
```
yampt.exe --convert --add-hyperlinks -f "C:\path\to\Plugin.esp" -d "ENtoPL.xml"
```
Then if in your used dictionaries exist record:
```
<record>
    <_id>DIAL</_id>
    <key>skin of the pearl</key>
    <val>skóra perły</val>
<record>
```
text:
```
Some text skin of the pearl some text.
```
will be converted to:
```
Some text skin of the pearl some text. [skóra perły]
```
### Making dictionary from plugin with all records unchanged
Use for translate everything.
```
yampt.exe --make-raw -f <list of plugins>
```
e.g.
```
yampt.exe --make-raw -f "C:\path\to\Plugin.esp"
```
### Making dictionary from plugin with all records, but with CELL, DIAL, BNAM and SCTX translation
Use for translate everything, but with little help.
```
yampt.exe --make-all -f <list of plugins> -d <list of dictionaries>
```
e.g.
```
yampt.exe --make-all -f "C:\path\to\Plugin.esp" -d "ENtoPL.xml"
```
### Making dictionary from plugin with records that don't exist in used dictionaries
Use for translate only new records added by plugin.
```
yampt.exe --make-not -f <list of plugins> -d <list of dictionaries>
```
e.g.
```
yampt.exe --make-not -f "C:\path\to\Plugin.esp" -d "ENtoPL.xml"
```
### Making dictionary with only changed text compared to records found in dictionaries.
Use for translate only modified records.
```
yampt.exe --make-changed -f <list of plugins> -d <list of dictionaries>
```
This step requires preparation of the helper dictionary.
```
yampt.exe --make-all -f "C:\path\to\EN\Morrowind.esm" -d "ENtoPL.xml"
yampt.exe --make-all -f "C:\path\to\EN\Tribunal.esm" -d "ENtoPL.xml"
yampt.exe --make-all -f "C:\path\to\EN\Bloodmoon.esm" -d "ENtoPL.xml"
```
```
yampt.exe --merge -d "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml" -o "ENtoPL_H.xml"
```
and finally:
```
yampt.exe --make-changed -f "C:\path\to\Plugin.esp" -d "ENtoPL_H.xml"
```
