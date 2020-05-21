@echo off
SETLOCAL EnableDelayedExpansion

SET modRamTotal = 0
SET modFlashTotal = 0
SET modGitTag=unknown (CMD git is not installed)

FOR /F "tokens=* USEBACKQ" %%a IN (`git -C %2 describe --tags --always --dirty 2^>nul`) DO SET modGitTag=%%a

FOR /F "tokens=1,2 skip=1" %%j in (%1) DO (
    echo %%j | findstr /R "^\.data ^\.rodata ^\.bss" >nul
    if !errorlevel! == 0 SET /A modRamTotal += %%k
    echo %%j | findstr /R "^\.irom0\.text ^\.text ^\.data ^\.rodata" >nul
    if !errorlevel! == 0 SET /A modFlashTotal += %%k
)

SET modRamTotal=            %modRamTotal%
SET modFlashTotal=           %modFlashTotal%

echo.
echo # Version
echo #  XS:    %modGitTag%
echo # Memory Usage
echo #  Ram:   %modRamTotal:~-6% bytes
echo #  Flash: %modFlashTotal:~-6% bytes
echo.

SET modRamTotal=
SET modFlashTotal=
SET modGitTag=
