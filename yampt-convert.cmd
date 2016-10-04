REM Set path to folders where you keep native and foreign master files (Morrowind.esm, Tribunal.esm and Bloodmoon.esm)
SET PATH_NATIVE=
SET PATH_FOREIGN=

REM Set path to your Wrye Mash installers folder or where you keep files (script process recursively from this path)
SET PATH_PLUGIN=

REM Skip
SET BASE=dictionaries_base
SET USER=dictionaries_user
SET NEW=dictionaries_new
SET OUT=converted_files

REM Change path to base dictionaries if you use modified
SET DICT_NATIVE=%BASE%\NATIVE.dic
SET DICT_FOREIGN=%BASE%\FOREIGN.dic

REM ############### DON'T EDIT ###############

mkdir %BASE%
mkdir %USER%
mkdir %NEW%
mkdir %OUT%

del /f /q %NEW%\*
del /f /q %OUT%\*

REM Create new native dictionary
yampt.exe --make-base -f "%PATH_NATIVE%\Morrowind.esm" "%PATH_FOREIGN%\Morrowind.esm"
yampt.exe --make-base -f "%PATH_NATIVE%\Tribunal.esm" "%PATH_FOREIGN%\Tribunal.esm"
yampt.exe --make-base -f "%PATH_NATIVE%\Bloodmoon.esm" "%PATH_FOREIGN%\Bloodmoon.esm"
yampt.exe --merge -d "Morrowind.dic" "Tribunal.dic" "Bloodmoon.dic" -o "NATIVE.dic"

del "Morrowind.dic" "Tribunal.dic" "Bloodmoon.dic"
move "NATIVE.dic" %BASE%

REM Create new foreign dictionary
yampt.exe --make-all -f "%PATH_FOREIGN%\Morrowind.esm" -d "%BASE%\NATIVE.dic"
yampt.exe --make-all -f "%PATH_FOREIGN%\Tribunal.esm" -d "%BASE%\NATIVE.dic"
yampt.exe --make-all -f "%PATH_FOREIGN%\Bloodmoon.esm" -d "%BASE%\NATIVE.dic"
yampt.exe --merge -d "Morrowind.ALL.dic" "Tribunal.ALL.dic" "Bloodmoon.ALL.dic" -o "FOREIGN.dic"

del "Morrowind.ALL.dic" "Tribunal.ALL.dic" "Bloodmoon.ALL.dic"
move "FOREIGN.dic" %BASE%

REM Convert native master files
for /R "%PATH_NATIVE%" %%f in (*.esm) do echo | set /p name=" "%%f" " >> master_list.txt
for /f "delims=" %%x in (master_list.txt) do yampt.exe --convert -f %%x -d "%DICT_NATIVE%"

REM Make user dictionaries from plugins
for /R "%PATH_PLUGIN%" %%f in (*.esp, *.esm, *.ess) do echo | set /p name=" "%%f" " >> plugin_list.txt
for /f "delims=" %%x in (plugin_list.txt) do yampt.exe --make-not -f %%x -d "%DICT_NATIVE%"
for /f "delims=" %%x in (plugin_list.txt) do yampt.exe --make-changed -f %%x -d "%DICT_FOREIGN%"

move "*.CHANGED.dic" %NEW%
move "*.NOTFOUND.dic" %NEW%

REM Convert plugins
for /f "delims=" %%x in (plugin_list.txt) do yampt.exe --convert --add-dial -f %%x -d "%DICT_NATIVE%" "%USER%\*.dic"

move "*.esm" %OUT%
move "*.esp" %OUT%
move "*.ess" %OUT%

del master_list.txt
del plugin_list.txt

pause
