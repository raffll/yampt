@echo off

set NATIVE=
set FOREIGN=
set NAME=XXtoXX

REM ############### DON'T EDIT ###############

set BASE=dict_base

yampt.exe --make-base -f "%NATIVE%\Morrowind.esm" "%FOREIGN%\Morrowind.esm"
yampt.exe --make-base -f "%NATIVE%\Tribunal.esm" "%FOREIGN%\Tribunal.esm"
yampt.exe --make-base -f "%NATIVE%\Bloodmoon.esm" "%FOREIGN%\Bloodmoon.esm"
yampt.exe --merge -d "Morrowind.BASE.xml" "Tribunal.BASE.xml" "Bloodmoon.BASE.xml" -o "%NAME%.xml"

del "Morrowind.BASE.xml" "Tribunal.BASE.xml" "Bloodmoon.BASE.xml" >nul 2>&1
move "%NAME%.xml" "%BASE%"

yampt.exe --make-all -f "%FOREIGN%\Morrowind.esm" -d "%BASE%\%NAME%.xml"
yampt.exe --make-all -f "%FOREIGN%\Tribunal.esm" -d "%BASE%\%NAME%.xml"
yampt.exe --make-all -f "%FOREIGN%\Bloodmoon.esm" -d "%BASE%\%NAME%.xml"
yampt.exe --merge -d "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml" -o "%NAME%_H.xml"

del "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml" >nul 2>&1
move "%NAME%_H.xml" "%BASE%"

pause
