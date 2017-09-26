@echo off
set this_dir=%~dp0
set vcbuildtools="C:\Program Files (x86)\Microsoft Visual C++ Build Tools\vcbuildtools.bat"

cmd /K "%vcbuildtools% x86 && cd /d %this_dir%"
