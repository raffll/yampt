# Easy start!

### Warning!
Make sure "yampt.exe" and all "cmd" scripts are in the same directory
and this directory doesn't contain any esm, esp, or xml files!

## Creating base dictionaries

In file "yampt-make-base-template.cmd" change lines:

    SET _NATIVE=
    SET FOREIGN=

to path where you keep native and foreign master files (e.g. Morrowind.esm, Tribunal.esm, Bloodmoon.esm)

    SET _NATIVE=C:\path\to\native
    SET FOREIGN=C:\path\to\foreign

Then double-click on script. This creates your base dictionaries in folder "dict\_base".

## Converting multiple files at once

You can copy all original foreign plugins to folder "input" and double-click on "yampt-convert-template.cmd".
This option creates converted files in folder "output".

## Creating user dictionaries

When you run "yampt-make-user-template.cmd", script creates:

#### "dict\_new" folder contains:

- *.CHANGED.xml dictionaries with changed text in comparison with original,
previously created "NATIVE\_for\_find\_changed\_only.xml".
- *.NOTFOUND.xml dictionaries with text not found in any dictionary, e.g. new text added by plugin.

This folder is refreshed every time you run script.
You can now translate this files and copy them to:

#### "dict\_user" folder

Dictionaries in this folder are combined with base dictionary during next convertion.
