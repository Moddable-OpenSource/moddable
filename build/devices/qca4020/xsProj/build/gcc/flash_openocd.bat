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
SETLOCAL EnableDelayedExpansion

REM Get the paths from the environment variables if they are set.

IF NOT "%CLIENT_PATH%"=="" SET GDB_PATH=%CLIENT_PATH%\arm-none-eabi-gdb.exe

Taskkill /IM openocd.exe /F 2> nul
START /B openocd.exe -f qca402x_openocd.cfg
rmdir /S /Q gdbout
mkdir gdbout

SET ROOTDIR=..\..\..\..\..
SET SCRIPTDIR=%ROOTDIR%\build\tools\flash
SET M4_DIR=.\output
SET M4_IMAGE="%M4_DIR%\Quartz_HASHED.elf"

SET M0_DIR=%ROOTDIR%\bin\cortex-m0\threadx\
SET M0_IMAGE="%M0_DIR%\ioe_ram_m0_threadx_ipt.mbn"

REM Set WLAN_IMAGE=none (or any non-existent filename) to avoid programming a WLAN image.
SET WLAN_DIR=%ROOTDIR%\bin\wlan
SET WLAN_IMAGE="%WLAN_DIR%\wlan_fw_img.bin"

REM if gdb client path is not set, try to find it
IF "%GDB_PATH%"=="" (
	where arm-none-eabi-gdb.exe > gdbclient.txt
	SET /P GDB_PATH=<gdbclient.txt
	for /f %%R in ("gdbclient.txt") do if %%~zR==0 (
		echo ERROR: GDB client path is missing!! Update PATH Environment variable
		goto :exit
	)
)


SET GDB_SERVER=localhost
SET GDB_PORT=3333

REM Enable Below if Signed images are used
REM SET M4_DIR=%ROOTDIR%\quartz\demo\QCLI_demo\build\gcc\4020\m4
REM SET M0_DIR=%ROOTDIR%\quartz\demo\QCLI_demo\build\gcc\4020\m0
REM SET WLAN_DIR=%ROOTDIR%\quartz\demo\QCLI_demo\build\gcc\4020\kf
REM SET M4_IMAGE="%M4_DIR%\Quartz.elf"
REM SET M0_IMAGE="%M0_DIR%\ioe_ram_m0_threadx_ipt.mbn"
REM SET WLAN_IMAGE="4020\kf\wlan_fw_img.bin"

DEL /F firmware_table.bin
DEL /F generated_partition_table.xml
DEL /F generated_fwd_table.xml

DEL /F output\firmware_table.bin
DEL /F output\generated_partition_table.xml
DEL /F output\generated_fwd_table.xml

REM Create partition_table.xml
IF /I "%~1" == "ota" (
   rem python %SCRIPTDIR%\gen_part_table.py --file=%M4_IMAGE% --file=%M0_IMAGE%
   python %SCRIPTDIR%\gen_part_table.py --begin=12KB --partition --file=%M4_IMAGE%  ^
                   --partition --file=%M0_IMAGE% ^
                   --partition --id=FS1 --file=%OTA_IMAGE% ^
                   --partition --id=UNUSED --size=8KB --start=4KB
) ELSE (
   IF EXIST "%WLAN_IMAGE%" (
   rem     python %SCRIPTDIR%\gen_part_table.py  --file=%M4_IMAGE%  --file=%M0_IMAGE%  --file=%WLAN_IMAGE%
   python %SCRIPTDIR%\gen_part_table.py --begin=140KB --partition --file=%M4_IMAGE%  ^
                   --partition --file=%M0_IMAGE% ^
                   --partition --file=%WLAN_IMAGE% ^
                   --partition --id=FS1 --size=64KB --start=12KB ^
                   --partition --id=FS2 --size=64KB --start=76KB ^
                   --partition --id=UNUSED --size=8KB --start=4KB
   ) else (
   rem python %SCRIPTDIR%\gen_part_table.py --file=%M4_IMAGE% --file=%M0_IMAGE%
   python %SCRIPTDIR%\gen_part_table.py --begin=140KB --partition --file=%M4_IMAGE%  ^
                   --partition --file=%M0_IMAGE% ^
                   --partition --id=FS1 --size=64KB --start=12KB ^
                   --partition --id=FS2 --size=64KB --start=76KB ^
                   --partition --id=UNUSED --size=8KB --start=4KB
   )
)

IF errorlevel 1 (
    echo Abort flash.bat: gen_part_table.py failed
    goto exit
)

REM Convert to fwd_table.xml
python %SCRIPTDIR%\gen_fwd_table.py -x generated_partition_table.xml --rawprogram generated_fwd_table.xml

if errorlevel 1 (
     echo Abort flash.bat: gen_fwd_table.py failed
     goto exit
)

echo Starting GDB Server....

python %SCRIPTDIR%/flash_through_gdb.py ^
	--rawprogram=generated_fwd_table.xml ^
	--verbose=5 ^
	--verify ^
	--outputdir=gdbout ^
	--gdbport=%GDB_PORT% ^
	--gdbpath="%GDB_PATH%" ^
	--jtagprgelf=%SCRIPTDIR%\JTAGPROGRAMMER_IMG_ARNTRI.elf ^
	--search_path=%M4_DIR%,%M0_DIR%,%WLAN_DIR% ^
	--gdbserver %GDB_SERVER%:%GDB_PORT%

if errorlevel 1 (
    echo Abort flash.bat: flash_through_gdb.py failed
	Taskkill /IM openocd.exe /F 2> nul
    goto exit
) else (
	echo Flash Operation Completed Successfully...
	Taskkill /IM openocd.exe /F 2> nul
)

:exit
DEL /F gdbclient.txt 2> nul
DEL /F gdbserver.txt 2> nul
