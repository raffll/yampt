@echo off

REM ############### DON'T EDIT ###############

set INPUT=input
set OUTPUT=output
set BASE=dict_base
set NEW=dict_new
set DICT_N=%BASE%\NATIVE.xml
set DICT_F=%BASE%\NATIVE_for_find_changed_only.xml

del /f "%_NEW%\*.xml" >nul 2>&1

setlocal enabledelayedexpansion enableextensions

set ESM=
for %%f in ("%DATA%\*.esp", "%DATA%\*.esm") do set ESM=!ESM! "%%f"
set ESM=%ESM:~1%

yampt.exe --make-not --add-hyperlinks -f %ESM% -d "%DICT_N%" )
yampt.exe --make-changed --add-hyperlinks -f %ESM% -d "%DICT_F%" )

move /y "*.CHANGED.xml" "%_NEW%" >nul 2>&1
move /y "*.NOTFOUND.xml" "%_NEW%" >nul 2>&1

pause
