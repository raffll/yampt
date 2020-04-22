@echo off

SET _INPUT=input
SET OUTPUT=output

SET BASE=dict_base
SET _NEW=dict_new

SET DICT_N=%BASE%\NATIVE.xml
SET DICT_F=%BASE%\NATIVE_for_find_changed_only.xml

REM ############### DON'T EDIT ###############

mkdir "%_INPUT%" >nul 2>&1
mkdir "%OUTPUT%" >nul 2>&1
mkdir "%BASE%" >nul 2>&1
mkdir "%_NEW%" >nul 2>&1

del tmp1.txt >nul 2>&1
del tmp2.txt >nul 2>&1
del /f "%_NEW%\*.xml" >nul 2>&1

for /r "%_INPUT%" %%f in (*.esp, *.esm) do echo %%f >> tmp1.txt
for /f "delims=" %%x in (tmp1.txt) do ( echo | set /p name=" "%%x" " ) >> tmp2.txt
for /f "delims=" %%x in (tmp2.txt) do ( yampt.exe --make-not --add-hyperlinks -f %%x -d "%DICT_N%" )
for /f "delims=" %%x in (tmp2.txt) do ( yampt.exe --make-changed --add-hyperlinks -f %%x -d "%DICT_F%" )

del tmp1.txt >nul 2>&1
del tmp2.txt >nul 2>&1

move /y "*.CHANGED.xml" "%_NEW%" >nul 2>&1
move /y "*.NOTFOUND.xml" "%_NEW%" >nul 2>&1

pause
