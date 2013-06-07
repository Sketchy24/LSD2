echo off
set TCL_LIBRARY=%CD%\gnu\lib\tcl8.4
cd > src\cd.txt
path > src\cp.txt
src\setpath.exe
del src\cd.txt
del src\cp.txt
src\kicklmm.bat %1


