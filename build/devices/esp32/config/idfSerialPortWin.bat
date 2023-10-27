@echo off

set CACHEDIR=%MODDABLE%\build\tmp\esp32
set OUTPUTFILE=%CACHEDIR%\UPLOAD_PORT

call %IDF_PATH%\export.bat > nul 2>&1
python.exe %IDF_PATH%\components\esptool_py\esptool\esptool.py --connect-attempts 1 flash_id | FINDSTR /b "Serial Port" > %OUTPUTFILE% 2>nul
FOR /F "tokens=3" %%a in (%OUTPUTFILE%) DO SET PORT=%%a
echo %PORT% > %OUTPUTFILE%
