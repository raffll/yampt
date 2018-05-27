# Easy start!

### Warning!
Make sure "yampt.exe", "yampt-convert-vanilla.cmd" and "yampt-make-base.cmd" are in the same directory and this directory doesn't contain any esm, esp, or xml files!

## Creating base dictionaries

In file "yampt-make-base.cmd" change lines:
```
SET _NATIVE=
SET FOREIGN=
```
to path where you keep native and foreign master files (e.g. Morrowind.esm, Tribunal.esm, Bloodmoon.esm)
```
SET _NATIVE=C:\path\to\native
SET FOREIGN=c:\path\to\foreign
```
Then double-click on script. This creates your base dictionaries in folder "dict_base".
