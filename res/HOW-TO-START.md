# Easy start!

### Warning!
Make sure "yampt.exe", "yampt-convert-template.cmd" and "yampt-make-base-template.cmd" are in the same directory and this directory doesn't contain any esm, esp, or xml files!

## Creating base dictionaries

In file "yampt-make-base-template.cmd" change lines:

    SET _NATIVE=
    SET FOREIGN=

to path where you keep native and foreign master files (e.g. Morrowind.esm, Tribunal.esm, Bloodmoon.esm)

    SET _NATIVE=C:\path\to\native
    SET FOREIGN=c:\path\to\foreign

Then double-click on script. This creates your base dictionaries in folder "dict\_base".

## Converting multiple files at once

#### You can create folder "input", copy all original foreign plugins and double-click on "yampt-convert-template.cmd".

This option creates folder "output" with converted files.

#### You can edit "yampt-convert-template.cmd" to search for plugins in different path, and/or copy converted to different destination, e.g.:

    SET _INPUT=input
    SET OUTPUT=output

to:

    SET _INPUT=C:\Morrowind\Installers
    SET OUTPUT=C:\Morrowind\Data Files

This option search for all esp and esm files in Wrye Mash "Installers" folder and copy converted directly to "Data Files" overwriting existing ones.

#### You can edit "yampt-convert-template.cmd" to search for plugins in different path, and/or copy converted to different destination with changing suffix, e.g.:

    SET _INPUT=input
    SET OUTPUT=output

    REM Optional
    SET SUFFIX=

to:

    SET _INPUT=C:\Morrowind\Data Files
    SET OUTPUT=C:\Morrowind\Data Files

    REM Optional
    SET SUFFIX=.CONV

This option search for all esp and esm files in "Data Files" folder and copy converted directly to "Data Files". Converted ones now have ".CONV" suffix (Morrowind.CONV.esm). This change also includes plugin dependency list, so "Plugin.CONV.esp" now depends on "Morrowind.CONV.esm". This creates entire separate plugin tree.

### Optional

Whatever option you choose, "yampt-convert-template.cmd" creates:

#### "dict\_new" folder contains:

- CHANGED.xml dictionaries with changed text in comparison with original, previously created "NATIVE\_for\_find\_changed\_only.xml".
- NOTFOUND.xml dictionaries with text not found in any dictionary, e.g. new text added by plugin.

This folder is refreshed every time you run script.
You can now translate this files and copy them to:

#### "dict\_user" folder

Dictionaries in this folder are combined with base dictionary during next convertion.
