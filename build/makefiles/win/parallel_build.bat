@echo off
REM
REM Copyright (c) 2023 Moddable Tech, Inc.
REM
REM   This file is part of the Moddable SDK Tools.
REM 
REM   The Moddable SDK Tools is free software: you can redistribute it and/or modify
REM   it under the terms of the GNU General Public License as published by
REM   the Free Software Foundation, either version 3 of the License, or
REM   (at your option) any later version.
REM 
REM   The Moddable SDK Tools is distributed in the hope that it will be useful,
REM   but WITHOUT ANY WARRANTY; without even the implied warranty of
REM   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
REM   GNU General Public License for more details.
REM 
REM   You should have received a copy of the GNU General Public License
REM   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
REM
REM
SET BUILD_DIR=%MODDABLE%\build
SET XS_DIR=%MODDABLE%\xs

IF "%1%"=="clean" (
    echo Cleaning...
    rmdir /S /Q %BUILD_DIR%\tmp
    rmdir /S /Q %BUILD_DIR%\bin
    echo Done.
    EXIT /B 0
)

@echo This multi-processor build will open several CMD windows for parallel builds. Do not press any keys until the build is complete.
@echo If you experience any issues with this build, please use build.bat instead to view the build output.
(
    start "Building debug xsc" cmd /C "nmake GOAL=debug BUILD_DIR=%BUILD_DIR% XS_DIR=%XS_DIR% /c /f %XS_DIR%\makefiles\win\xsc.mak /s"
    start "Building debug xsid" cmd /C "nmake GOAL=debug BUILD_DIR=%BUILD_DIR% XS_DIR=%XS_DIR% /c /f %XS_DIR%\makefiles\win\xsid.mak /s"
    start "Building debug xsl" cmd /C "nmake GOAL=debug BUILD_DIR=%BUILD_DIR% XS_DIR=%XS_DIR% /c /f %XS_DIR%\makefiles\win\xsl.mak /s"
    start "Building release xsc" cmd /C "nmake GOAL=release BUILD_DIR=%BUILD_DIR% XS_DIR=%XS_DIR% /c /f %XS_DIR%\makefiles\win\xsc.mak /s"
    start "Building release xsid" cmd /C "nmake GOAL=release BUILD_DIR=%BUILD_DIR% XS_DIR=%XS_DIR% /c /f %XS_DIR%\makefiles\win\xsid.mak /s"
    start "Building release xsl" cmd /C "nmake GOAL=release BUILD_DIR=%BUILD_DIR% XS_DIR=%XS_DIR% /c /f %XS_DIR%\makefiles\win\xsl.mak /s"
) | set /P delay=Building xs tools, please wait...
@echo done


(
    start "Building debug serial2xsbug" cmd /C "nmake GOAL=debug BUILD_DIR=%BUILD_DIR% XS_DIR=%XS_DIR% /c /f serial2xsbug.mak /s"
    start "Building debug tools" cmd /C "nmake GOAL=debug BUILD_DIR=%BUILD_DIR% XS_DIR=%XS_DIR% /c /f tools.mak /s"
    start "Building release serial2xsbug" cmd /C "nmake GOAL=release BUILD_DIR=%BUILD_DIR% XS_DIR=%XS_DIR% /c /f serial2xsbug.mak /s"
    start "Building release tools" cmd /C "nmake GOAL=release BUILD_DIR=%BUILD_DIR% XS_DIR=%XS_DIR% /c /f tools.mak /s"
) | set /P delay=Building sdk tools, please wait...
@echo done

(
    start "Building debug xsbug" cmd /C "call %MODDABLE%\build\bin\win\debug\mcconfig -d -m -p x-win %MODDABLE%\tools\xsbug\manifest.json"
    start "Building release xsbug" cmd /C "call %MODDABLE%\build\bin\win\release\mcconfig -m -p x-win %MODDABLE%\tools\xsbug\manifest.json"
    start "Building debug mcsim" cmd /C "call %MODDABLE%\build\bin\win\release\mcconfig -d -m -p x-win %MODDABLE%\tools\mcsim\manifest.json"
    start "Building release mcsim" cmd /C "call %MODDABLE%\build\bin\win\release\mcconfig -m -p x-win %MODDABLE%\tools\mcsim\manifest.json"
) | set /P delay=Building xsbug, please wait...
@echo done
@echo Build complete.