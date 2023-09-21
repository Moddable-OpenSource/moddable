echo off

IF %1 == "MODDABLE4" (
	set deviceName=Moddable Four
) else (
	set deviceName=nRF52
)

echo Waiting for volume %deviceName%.

REM Check for mounted Moddable Four volume.
wmic logicaldisk get caption, description, VolumeName | (find /i "%1" > %2)
IF %ERRORLEVEL% == 0 (GOTO Done)

REM Check for Moddable Four debug build serial device.
wmic path Win32_SerialPort where "PNPDeviceID like '%%VID_%4&PID_%5%%'" get DeviceID 2> nul | (find /i "COM" > %3)
IF NOT %ERRORLEVEL% == 0 (GOTO NoDebugPort)

echo Found %deviceName% device. Putting the device into programming mode.

echo | set moddableFourTemp=If device LED does not start blinking, please double-tap the reset button to enter programming mode manually.

for /F "tokens=1" %%i in ( %3 ) do serial2xsbug %%i 921600 8N1 -programming
timeout /T 5 /NOBREAK > nul
GOTO CheckForVolume

:NoDebugPort
echo | set /p moddableFourTemp=%deviceName% device not found or release build installed. Please connect your %deviceName% via USB and then double-tap the reset button to enter programming mode.

:CheckForVolume
wmic logicaldisk get caption, description, VolumeName | (find /i "%1" > %2)
IF %ERRORLEVEL% == 0 (GOTO Done)

echo | set /p moddableFourTemp=.
timeout /T 1 /NOBREAK > nul
GOTO CheckForVolume
:Done
echo %deviceName% volume found.

set  moddableFourTemp=
