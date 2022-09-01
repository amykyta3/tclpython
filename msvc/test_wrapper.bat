@echo off

set "TCL_PATH=%~1"
set "OUTPUT_DIR=%~2"
set "PYTHON_PATH=%~3"

set "TCL_EXE=%TCL_PATH%\bin\tclsh.exe"
set TCLLIBPATH="%cd:\=/%/%OUTPUT_DIR:\=/%"
set "PATH=%PYTHON_PATH%;%PATH%"

%TCL_EXE% ..\test\test.tcl
