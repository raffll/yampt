REM Set path to folders where you keep native and foreign master files (Morrowind.esm, Tribunal.esm and Bloodmoon.esm)
SET _NATIVE=
SET FOREIGN=

SET BASE=dict_base

REM ############### DON'T EDIT ###############

REM Prepare
mkdir %BASE%

REM Create new native dictionaries
yampt.exe --make-base -f "%_NATIVE%\Morrowind.esm" "%FOREIGN%\Morrowind.esm"
yampt.exe --make-base -f "%_NATIVE%\Tribunal.esm" "%FOREIGN%\Tribunal.esm"
yampt.exe --make-base -f "%_NATIVE%\Bloodmoon.esm" "%FOREIGN%\Bloodmoon.esm"
yampt.exe --merge -d "Morrowind.BASE.xml" "Tribunal.BASE.xml" "Bloodmoon.BASE.xml" -o "NATIVE.xml"

del "Morrowind.BASE.xml" "Tribunal.BASE.xml" "Bloodmoon.BASE.xml"
move "NATIVE.xml" "%BASE%"
rename "yampt.log" "yampt.NATIVE.log"

yampt.exe --make-all -f "%FOREIGN%\Morrowind.esm" -d "%BASE%\NATIVE.xml"
yampt.exe --make-all -f "%FOREIGN%\Tribunal.esm" -d "%BASE%\NATIVE.xml"
yampt.exe --make-all -f "%FOREIGN%\Bloodmoon.esm" -d "%BASE%\NATIVE.xml"
yampt.exe --merge -d "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml" -o "NATIVE_for_find_changed_only.xml"

del "Morrowind.ALL.xml" "Tribunal.ALL.xml" "Bloodmoon.ALL.xml"
move "NATIVE_for_find_changed_only.xml" "%BASE%"
rename "yampt.log" "NATIVE_for_find_changed_only.log"

pause
