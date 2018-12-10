@echo off

SET BASE=dict_base
SET USER=dict_user

SET DICT_N=%BASE%\NATIVE.xml

SET ECHO_1=Finding errors and duplicates in DIAL and CELL records...
SET ECHO_2=Finding unused INFO records...

REM ############### DON'T EDIT ###############

mkdir "%BASE%" >nul 2>&1
mkdir "%USER%" >nul 2>&1

del tmp1.txt >nul 2>&1
del raport.log >nul 2>&1

for /r "%USER%" %%f in (*.xml) do echo %%f >> tmp1.txt

echo %ECHO_1%
echo %ECHO_1% >> raport.log
for /f "delims=" %%x in (tmp1.txt) do ( yampt.exe --merge -l -d %%x | findstr "Loading Duplicate Too Ok Invalid Doubled" | findstr /v /r /c:"Doubled.*Invalid" ) >> raport.log
echo %ECHO_2%
echo %ECHO_2% >> raport.log
for /f "delims=" %%x in (tmp1.txt) do ( yampt.exe --merge -l -d "%DICT_N%" %%x | findstr "Loading Unused" ) >> raport.log

del tmp1.txt >nul 2>&1
del Merged.xml >nul 2>&1

pause
