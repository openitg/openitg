@echo off

rem ###if a new patch directory exists, then move it###
if exist Data\new-patch move Data\new-patch Data\patch

rem ###Start OpenITG###
if exist Data\patch\OpenITG-PC.exe Data\patch\OpenITG-PC.exe --type=Preferences-cabinet
else Program\OpenITG-PC.exe --type=Preferences-cabinet