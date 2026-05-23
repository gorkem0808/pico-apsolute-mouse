@echo off
set SCRIPT=%~dp0MASAUSTU_PROGRAMI_BASLAT.bat
set STARTUP=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup
copy "%SCRIPT%" "%STARTUP%\GT_ARCADE_KONTROL_IO_BOARD_V001.bat" /Y
echo Baslangica eklendi.
pause
