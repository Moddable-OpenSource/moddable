@ECHO OFF
REM Copyright (c) 2016-2018 Qualcomm Technologies, Inc.
REM 2016 Qualcomm Atheros, Inc.
REM All Rights Reserved
REM Copyright (c) 2018 Qualcomm Technologies, Inc.
REM All rights reserved.
REM Redistribution and use in source and binary forms, with or without modification, are permitted (subject to the limitations in the disclaimer below)
REM provided that the following conditions are met:
REM Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
REM Redistributions in binary form must reproduce the above copyright notice,
REM this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
REM Neither the name of Qualcomm Technologies, Inc. nor the names of its contributors may be used to endorse or promote products derived
REM from this software without specific prior written permission.
REM NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
REM THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
REM BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
REM IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
REM OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
REM LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
REM WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
REM EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

REM Note to use this debug script, the JlinkGDBServer and arm-non-eabi-gdb executables
REM needs to be in the system path. If either of these isn't in the system path,
REM JLINK_PATH and/or GCC_BIN_PATH must be defined with their location.

IF /I "%CHIPSET_VERSION%" == "v1" (SET CHIPSET=v1) ELSE (SET CHIPSET=v2)

IF /I "%CHIPSET%" == "v1" (
    @ECHO ****************************************************************************
    @ECHO    Starting M4 Debug session for Quartz QCLI Application for v1 Chipset
    @ECHO                  To debug v2, set CHIPSET_VERSION=v2                  
    @ECHO *****************************************************************************
) ELSE (
    @ECHO ****************************************************************************
    @ECHO    Starting M4 Debug session for Quartz QCLI Application for v2 Chipset
    @ECHO                  To debug v1, set CHIPSET_VERSION=v1                  
    @ECHO *****************************************************************************
)

REM Get the paths from the environment variables if they are set.
SET SERVER_PATH=
SET CLIENT_PATH=
IF NOT "%JLINK_PATH%"   == "" SET SERVER_PATH=%JLINK_PATH:"=%\
if NOT "%GCC_BIN_PATH%" == "" SET CLIENT_PATH=%GCC_BIN_PATH:"=%\

REM Set the options for the Jlink GDB server
SET JLinkOptions=-scriptfile "%~dp0%Quartz.JLinkScript"
SET JLinkOptions=%JLinkOptions% -select USB
SET JLinkOptions=%JLinkOptions% -device Cortex-M4
SET JLinkOptions=%JLinkOptions% -endian little
SET JLinkOptions=%JLinkOptions% -if JTAG
SET JLinkOptions=%JLinkOptions% -speed 1000
SET JLinkOptions=%JLinkOptions% -noir
SET JLinkOptions=%JLinkOptions% -singlerun
SET JLinkOptions=%JLinkOptions% -port 2331

REM Start the GDB Server.
START "JLinkGDBServer" /MIN "%SERVER_PATH%JLinkGDBServerCL.exe" %JLinkOptions%

REM Start the GDB Client
IF /I "%1" == "asic" (
    @ECHO Running ASIC gdb script
    "arm-none-eabi-gdb.exe" -x "%CHIPSET%\m4asic.gdbinit"
) ELSE IF  /I "%1" == "m4" (
    @ECHO Running ASIC gdb script for m4-only configuration
    "arm-none-eabi-gdb.exe" -x "%CHIPSET%\quartzasic.gdbinit"
) ELSE (
    @ECHO Running Emulation gdb script for v2 configuration
    "arm-none-eabi-gdb.exe" -x "%CHIPSET%\Quartz_m4.gdbinit"
)

