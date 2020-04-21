@echo off

SET DATA=..\Data Files
SET BASE=dict_base
SET USER=dict_user
SET DICT_N=%BASE%\ENtoPL_plpatch.xml
SET SUFFIX= (plpatch)

REM ############### DON'T EDIT ###############

mkdir "%DATA%" >nul 2>&1
mkdir "%BASE%" >nul 2>&1
mkdir "%USER%" >nul 2>&1

del /f "%DATA%\*%SUFFIX%*" >nul 2>&1
del mods.txt >nul 2>&1
del *.log >nul 2>&1

ren "%DATA%\Morrowind.esm.bak" Morrowind.esm

for /r "%DATA%" %%f in (*.esp, *.esm) do ( echo | set /p name=" "%%f" " ) >> mods.txt
for /f "delims=" %%x in (mods.txt) do ( yampt.exe --convert --add-hyperlinks --windows-1250 -f %%x -d "%DICT_N%" %USER%\*.xml -s "%SUFFIX%" )

ren "%DATA%\Morrowind.esm" Morrowind.esm.bak
del mods.txt >nul 2>&1

move /y "*.esm" "%DATA%" >nul 2>&1
move /y "*.esp" "%DATA%" >nul 2>&1

pause
