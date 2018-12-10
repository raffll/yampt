@echo off

SET _INPUT=input
SET OUTPUT=output
SET SUFFIX=

SET BASE=dict_base
SET USER=dict_user

SET DICT_N=%BASE%\NATIVE.xml

REM ############### DON'T EDIT ###############

mkdir "%_INPUT%" >nul 2>&1
mkdir "%OUTPUT%" >nul 2>&1
mkdir "%BASE%" >nul 2>&1
mkdir "%USER%" >nul 2>&1

del tmp1.txt >nul 2>&1
del /f "%OUTPUT%\*%SUFFIX%*" >nul 2>&1

for /r "%_INPUT%" %%f in (*.esp, *.esm) do ( echo | set /p name=" "%%f" " ) >> tmp1.txt
for /f "delims=" %%x in (tmp1.txt) do ( yampt.exe --convert -a -f %%x -d "%DICT_N%" %USER%\*.xml -s "%SUFFIX%" )

del tmp1.txt >nul 2>&1

move /y "*.esm" "%OUTPUT%" >nul 2>&1
move /y "*.esp" "%OUTPUT%" >nul 2>&1

pause
