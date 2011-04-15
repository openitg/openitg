@rem Delete temporary folder before setting anything up
@rmdir /s /q inst-tmp

@rem Move everything we want into a single directory
mkdir inst-tmp
xcopy /y /e assets\d4\* inst-tmp
xcopy /y /e assets\game-data\* inst-tmp
mkdir inst-tmp\Data\patch
mkdir inst-tmp\Data\patch\patch
xcopy /y /e /exclude:assets\patch-data\zip.sh assets\patch-data\* inst-tmp\Data\patch\patch
xcopy /y /e /i Program inst-tmp\Program
xcopy /y assets\win32-installer\ASF.ini inst-tmp
xcopy /y assets\win32-installer\*.ico inst-tmp\Data
xcopy /y assets\win32-installer\*.png inst-tmp\Data
xcopy /y ReleaseNotes.txt inst-tmp
xcopy /y WhoToSue.txt inst-tmp

@rem Do some pruning: 
@rem  1. remove cache and .sm files to songs we don't have
@rem  2. remove zip.sh
@rmdir /s /q inst-tmp\Cache
@rmdir /s /q inst-tmp\Songs
@rmdir /s /q inst-tmp\Themes\ps2onpc
@rmdir /s /q inst-tmp\Themes\ps2
@del /q /f inst-tmp\zip.sh


rem makensis /nocd assets\win32-installer\OpenITG.nsi
