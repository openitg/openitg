@echo off

rem if a new patch directory exists, then move it
if exist Data\new-patch move Data\new-patch Data\patch

rem Start OpenITG
Program\OpenITG-PC.exe --type=Preferences-cabinet