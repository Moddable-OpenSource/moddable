@echo Waiting for volume %1.
@echo | set /p moddableFourTemp=Double-tap the reset button to enter programming mode.
:DoWhile
@wmic logicaldisk get caption, description, VolumeName | (find /i "%1" > %2)
@IF %ERRORLEVEL% == 0 (GOTO Done)
@echo | set /p moddableFourTemp=.
@timeout /T 1 /NOBREAK > nul
@GOTO DoWhile
:Done
@echo volume found