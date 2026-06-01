@echo off

set EXE=x64\Release\yampt.exe
set NATIVE=master\pl\Morrowind.esm
set FOREIGN=master\en\Morrowind.esm

%EXE% --make-base -f "%NATIVE%" "%FOREIGN%"
%EXE% --make -f "%NATIVE%"

if "%1"=="nopause" goto end
pause
:end
