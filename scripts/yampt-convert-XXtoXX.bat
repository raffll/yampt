@echo off

set DICT_N=%BASE%\XXtoXX.xml

REM ############### DON'T EDIT ###############

set INPUT=input
set OUTPUT=output
set BASE=dict_base
set USER=dict_user

setlocal enabledelayedexpansion enableextensions

set DICT_U=
for %%x in ("%USER%\*.xml") do set DICT_U=!DICT_U! "%%x"
set DICT_U=%DICT_U:~1%

set ESM=
for %%f in ("%INPUT%\*.esp", "%INPUT%\*.esm") do set ESM=!ESM! "%%f"
set ESM=%ESM:~1%

yampt.exe --convert --add-hyperlinks -f %ESM% -d "%DICT_N%" %DICT_U%

move /y "*.esm" "%OUTPUT%" >nul 2>&1
move /y "*.esp" "%OUTPUT%" >nul 2>&1

pause
