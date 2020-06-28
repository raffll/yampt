@echo off

REM ############### DON'T EDIT ###############

set INPUT=input
set OUTPUT=output
set BASE=dict_base
set NEW=dict_new
set DICT_N=%BASE%\NATIVE.xml
set DICT_F=%BASE%\NATIVE_for_find_changed_only.xml
set GLOS=%BASE%\Glossary.xml

del /f "%NEW%\*.xml" >nul 2>&1

setlocal enabledelayedexpansion enableextensions

set ESM=
for %%f in ("%INPUT%\*.esp", "%INPUT%\*.esm") do set ESM=!ESM! "%%f"
set ESM=%ESM:~1%

yampt.exe --make-not --add-hyperlinks -f %ESM% -d "%DICT_N%" "%GLOS%"
yampt.exe --make-changed --add-hyperlinks -f %ESM% -d "%DICT_F%" "%GLOS%"

move /y "*.CHANGED.xml" "%NEW%" >nul 2>&1
move /y "*.NOTFOUND.xml" "%NEW%" >nul 2>&1

pause
