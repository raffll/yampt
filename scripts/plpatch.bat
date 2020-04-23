@echo off

set DATA=..\Data Files
set BASE=dict_base
set USER=dict_user
set DICT_N=%BASE%\ENtoPL_plpatch.xml
set SUFFIX= (plpatch)

REM ############### DON'T EDIT ###############

mkdir "%DATA%" >nul 2>&1
mkdir "%BASE%" >nul 2>&1
mkdir "%USER%" >nul 2>&1

del /f "%DATA%\*%SUFFIX%*" >nul 2>&1

ren "%DATA%\Morrowind.esm.bak" Morrowind.esm

setlocal enabledelayedexpansion enableextensions

set DICT_U=
for %%x in ("%USER%\*.xml") do set DICT_U=!DICT_U! "%%x"
set DICT_U=%DICT_U:~1%

set ESM=
for %%f in ("%DATA%\*.esp", "%DATA%\*.esm") do set ESM=!ESM! "%%f"
set ESM=%ESM:~1%

yampt.exe --convert --add-hyperlinks --windows-1250 -f %ESM% -d "%DICT_N%" %DICT_U% -s "%SUFFIX%"

ren "%DATA%\Morrowind.esm" Morrowind.esm.bak

move /y "*.esm" "%DATA%" >nul 2>&1
move /y "*.esp" "%DATA%" >nul 2>&1

pause
