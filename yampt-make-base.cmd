REM Set path to folders where you keep native and foreign master files (Morrowind.esm, Tribunal.esm and Bloodmoon.esm)
SET PATH_NATIVE=
SET PATH_FOREIGN=

REM Skip
SET BASE=dict_base

REM ############### DON'T EDIT ###############

mkdir %BASE%

REM Create new native dictionary
yampt.exe --make-base -f "%PATH_NATIVE%\Morrowind.esm" "%PATH_FOREIGN%\Morrowind.esm"
yampt.exe --make-base -f "%PATH_NATIVE%\Tribunal.esm" "%PATH_FOREIGN%\Tribunal.esm"
yampt.exe --make-base -f "%PATH_NATIVE%\Bloodmoon.esm" "%PATH_FOREIGN%\Bloodmoon.esm"
yampt.exe --merge -d "Morrowind.dic" "Tribunal.dic" "Bloodmoon.dic" -o "NATIVE.dic"

del "Morrowind.dic" "Tribunal.dic" "Bloodmoon.dic"
move "NATIVE.dic" %BASE%
rename "yampt.log" "yampt.NATIVE.log"

REM Create new foreign dictionary
yampt.exe --make-all -f "%PATH_FOREIGN%\Morrowind.esm" -d "%BASE%\NATIVE.dic"
yampt.exe --make-all -f "%PATH_FOREIGN%\Tribunal.esm" -d "%BASE%\NATIVE.dic"
yampt.exe --make-all -f "%PATH_FOREIGN%\Bloodmoon.esm" -d "%BASE%\NATIVE.dic"
yampt.exe --merge -d "Morrowind.ALL.dic" "Tribunal.ALL.dic" "Bloodmoon.ALL.dic" -o "FOREIGN.dic"

del "Morrowind.ALL.dic" "Tribunal.ALL.dic" "Bloodmoon.ALL.dic"
move "FOREIGN.dic" %BASE%
rename "yampt.log" "yampt.FOREIGN.log"

pause
