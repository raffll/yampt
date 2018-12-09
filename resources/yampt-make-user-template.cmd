@echo off

SET _INPUT=input
SET OUTPUT=output
SET SUFFIX=

SET BASE=dict_base
SET USER=dict_user
SET _NEW=dict_new

SET DICT_N=%BASE%\NATIVE.xml
SET DICT_F=%BASE%\NATIVE_for_find_changed_only.xml

SET EXCLUDE=

REM ############### DON'T EDIT ###############

mkdir "%_INPUT%" >nul 2>&1
mkdir "%OUTPUT%" >nul 2>&1
mkdir "%BASE%" >nul 2>&1
mkdir "%USER%" >nul 2>&1
mkdir "%_NEW%" >nul 2>&1

del tmp1.txt >nul 2>&1
del tmp2.txt >nul 2>&1
del /f "%_NEW%\*.xml" >nul 2>&1

for /r "%_INPUT%" %%f in (*.esp, *.esm) do ( echo %%f | findstr /v "%EXCLUDE%" ) >> tmp1.txt
for /f "delims=" %%x in (tmp1.txt) do ( echo | set /p name=" "%%x" " ) >> tmp2.txt
for /f "delims=" %%x in (tmp2.txt) do ( yampt.exe --make-not -a -f %%x -d "%DICT_N%" )
for /f "delims=" %%x in (tmp2.txt) do ( yampt.exe --make-changed -a -f %%x -d "%DICT_F%" )

del tmp1.txt >nul 2>&1
del tmp2.txt >nul 2>&1

move /y "*.CHANGED.xml" "%_NEW%" >nul 2>&1
move /y "*.NOTFOUND.xml" "%_NEW%" >nul 2>&1

pause
