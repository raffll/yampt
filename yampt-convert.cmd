REM Set path to your Wrye Mash installers folder or where you keep files (script process recursively from this path)
SET PATH_PLUGIN=

REM Skip
SET BASE=dict_base
SET USER=dict_user
SET NEW=dict_new
SET OUT=output

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
