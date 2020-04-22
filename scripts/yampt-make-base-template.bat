@echo off

SET _NATIVE=
SET FOREIGN=

SET NAME=NATIVE

SET BASE=dict_base

REM ############### DON'T EDIT ###############

mkdir "%BASE%" >nul 2>&1

yampt.exe --make-base -f "%_NATIVE%\Morrowind.esm" "%FOREIGN%\Morrowind.esm"
yampt.exe --make-base -f "%_NATIVE%\Tribunal.esm" "%FOREIGN%\Tribunal.esm"
yampt.exe --make-base -f "%_NATIVE%\Bloodmoon.esm" "%FOREIGN%\Bloodmoon.esm"
yampt.exe --merge -d "Morrowind.BASE.xml" "Tribunal.BASE.xml" "Bloodmoon.BASE.xml" -o "%NAME%.xml"

del "Morrowind.BASE.xml" "Tribunal.BASE.xml" "Bloodmoon.BASE.xml" >nul 2>&1
move "%NAME%.xml" "%BASE%"

yampt.exe --make-all -f "%FOREIGN%\Morrowind.esm" -d "%BASE%\%NAME%.xml"
yampt.exe --make-all -f "%FOREIGN%\Tribunal.esm" -d "%BASE%\%NAME%.xml"
yampt.exe --make-all -f "%FOREIGN%\Bloodmoon.esm" -d "%BASE%\%NAME%.xml"
yampt.exe --merge -d "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml" -o "%NAME%_for_find_changed_only.xml"

del "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml" >nul 2>&1
move "%NAME%_for_find_changed_only.xml" "%BASE%"

pause
