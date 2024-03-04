@echo off
SETLOCAL EnableDelayedExpansion

SET /a SYMPATH=%1

SET addr = 0

FOR /F "tokens=1-3" %%A in (%1) DO (
	IF /I "%%C" == "__start_unused_space" (
		set /a "addr=0x%%A"
		GOTO :done
	)
)

:done

set /a "addr=%addr% + 4095"
set /a "mod=%addr% %% 4096"
set /a "addr=%addr% - %mod%"
set /a "addr=%addr% + 4096"

REM hack to convert dec to hex
call cmd /c exit /b %addr%
set hex=%=exitcode%
echo # mods start at 0x%hex%
