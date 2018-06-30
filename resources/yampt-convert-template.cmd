SET _INPUT=input
SET OUTPUT=output

REM Optional
SET SUFFIX=

SET BASE=dict_base
SET USER=dict_user
SET _NEW=dict_new

REM Change path to base dictionaries
SET DICT_N=%BASE%\NATIVE.xml
SET DICT_F=%BASE%\NATIVE_for_find_changed_only.xml

REM ############### DON'T EDIT ###############

REM Prepare
mkdir "%_INPUT%"
mkdir "%OUTPUT%"
mkdir "%BASE%"
mkdir "%USER%"
mkdir "%_NEW%"

del /f /q "%_NEW%"\*
for /R "%_INPUT%" %%f in (*.esp, *.esm, *.ESP, *.ESM) do echo | set /p name=" "%%f" " >> plg.txt

REM Make user dictionaries
for /f "delims=" %%x in (plg.txt) do ( yampt.exe --make-not -f %%x -d "%DICT_N%" )
for /f "delims=" %%x in (plg.txt) do ( yampt.exe --make-changed -f %%x -d "%DICT_F%" )

REM Convert files
for /f "delims=" %%x in (plg.txt) do ( yampt.exe --convert -a -f %%x -d "%DICT_N%" "%USER%\*.xml" -s "%SUFFIX%" )
	
REM Clean
move "*.CHANGED.xml" "%_NEW%"
move "*.NOTFOUND.xml" "%_NEW%"
move /Y "*.esm" "%OUTPUT%"
move /Y "*.esp" "%OUTPUT%"
move /Y "*.ESM" "%OUTPUT%"
move /Y "*.ESP" "%OUTPUT%"

del plg.txt

pause
