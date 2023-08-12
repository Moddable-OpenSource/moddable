@echo off
SETLOCAL EnableDelayedExpansion

REM 0x27000
SET SOFTDEVICE_ADDR=159744
REM 0x4200
SET SOFTDEVICE_RAM=16896
REM 0x2000
SET MOD_PREFS_SIZE=8192
REM 0xC000
SET BOOTLOADER_SIZE=49152

SET /a HEAP_SIZE=%3

SET modRamTotal = 0
SET modHeapTotal = 0
SET modFlashTotal = 0
SET modQSPITotal = 0
SET modGitTag=unknown (CMD git is not installed)

FOR /F "tokens=* USEBACKQ" %%a IN (`git -C %2 describe --tags --always --dirty 2^>nul`) DO SET modGitTag=%%a

FOR /F "tokens=1,2 skip=1" %%j in (%1) DO (
    echo %%j | findstr /R "^\.no_init ^\.data ^\.fs_data ^\.bss ^\.stack_dummy " >nul
    if !errorlevel! == 0 SET /A modRamTotal += %%k
    echo %%j | findstr /R "^\.text ^\.sdh ^\.crypto ^\.nrf ^\.ARM.exidx " >nul
    if !errorlevel! == 0 SET /A modFlashTotal += %%k
    echo %%j | findstr /R "^\.rodata.resources ^\.xs_lib_flash " >nul
	if "%3" == "1" (
	    if !errorlevel! == 0 SET /A modQSPITotal += %%k
	) else (
	    if !errorlevel! == 0 SET /A modFlashTotal += %%k
	)
)

SET /A modRamTotal=%modRamTotal%+%SOFTDEVICE_RAM%-%HEAP_SIZE%
SET /A modFlashTotal=%modFlashTotal%+%SOFTDEVICE_ADDR%+%MOD_PREFS_SIZE%+%BOOTLOADER_SIZE%

SET modRamTotal=            %modRamTotal%
SET modHeapTotal=           %HEAP_SIZE%
SET modFlashTotal=          %modFlashTotal%
SET modQSPITotal=           %modQSPITotal%

echo.
echo # Version
echo #  XS:    %modGitTag%
echo # Memory Usage
echo #  Ram:   %modRamTotal:~-6% bytes
echo #  Heap:  %modHeapTotal:~-6% bytes
echo #  Flash: %modFlashTotal:~-7% bytes
echo #  QSPI:  %modQSPITotal:~-7% bytes
echo.

SET modRamTotal=
SET modFlashTotal=
SET modGitTag=
