echo off

REM Wait for serial port with specified Vendor ID and Product ID.
:SerialWhile
wmic path Win32_SerialPort where "PNPDeviceID like '%%VID_%1&PID_%2%%'" get DeviceID 2> nul | (find /i "COM" > %3)
IF %ERRORLEVEL% == 0 (GOTO SerialDone)
GOTO SerialWhile
:SerialDone

REM type %3 2> nul
REM
