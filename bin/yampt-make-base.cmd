SET PATH_N=
SET PATH_F=

###########################################################################

yampt.exe --make-base -f "%PATH_N%\Morrowind.esm" "%PATH_F%\Morrowind.esm"
yampt.exe --make-base -f "%PATH_N%\Tribunal.esm" "%PATH_F%\Tribunal.esm"
yampt.exe --make-base -f "%PATH_N%\Bloodmoon.esm" "%PATH_F%\Bloodmoon.esm"

yampt.exe --merge -d "Morrowind.dic" "Tribunal.dic" "Bloodmoon.dic"

del "Morrowind.dic" "Tribunal.dic" "Bloodmoon.dic"

pause

