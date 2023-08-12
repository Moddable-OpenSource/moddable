echo off

REM Wait for the Moddable Four volume to unmount.
echo | set /p moddableFourTemp=Installing to %2.
:DoWhile
wmic logicaldisk get caption, description, VolumeName | (find /i "%2")
IF %ERRORLEVEL% NEQ 0 (GOTO Done)
echo | set /p moddableFourTemp=.
timeout /T 1 /NOBREAK > nul
GOTO DoWhile
:Done
echo .

IF %1 == 0 (echo Release build installed. && EXIT /b 0)

REM Wait for serial port with specified Vendor ID and Product ID.
:SerialWhile
wmic path Win32_SerialPort where "PNPDeviceID like '%%VID_%4&PID_%5%%'" get DeviceID 2> nul | (find /i "COM" > %3)
IF %ERRORLEVEL% == 0 (GOTO SerialDone)
GOTO SerialWhile
:SerialDone
echo Debug build installed.

set moddableFourTemp=
