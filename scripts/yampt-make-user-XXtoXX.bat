@echo off

set DICT=XXtoXX.xml
set DICT_H=XXtoXX_H.xml
set DICT_G=XXtoXX_G.xml
set GLOS=Glossary.xml
set COMMANDS=

REM ############### DON'T EDIT ###############

set INPUT=input
set OUTPUT=output
set BASE=dict_base
set NEW=dict_new

del /f "%NEW%\*.xml" >nul 2>&1

setlocal enabledelayedexpansion enableextensions

set ESM=
for %%f in ("%INPUT%\*.esp", "%INPUT%\*.esm") do set ESM=!ESM! "%%f"
set ESM=%ESM:~1%

yampt.exe --make-not %COMMANDS% -f %ESM% -d "%BASE%\%DICT%" "%BASE%\%DICT_G%" "%BASE%\%GLOS%"
yampt.exe --make-changed %COMMANDS% -f %ESM% -d "%BASE%\%DICT_H%" "%BASE%\%DICT_G%" "%BASE%\%GLOS%"

move /y "*.CHANGED.xml" "%NEW%" >nul 2>&1
move /y "*.NOTFOUND.xml" "%NEW%" >nul 2>&1

if "%1"=="nopause" goto start
pause
:start
