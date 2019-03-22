#!/bin/bash
# Copyright (c) 2016-2018 Qualcomm Technologies, Inc.
# All Rights Reserved.
# Copyright (c) 2018 Qualcomm Technologies, Inc.
# All rights reserved.
# Redistribution and use in source and binary forms, with or without modification, are permitted (subject to the limitations in the disclaimer below)
# provided that the following conditions are met:
# Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
# Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
# Neither the name of Qualcomm Technologies, Inc. nor the names of its contributors may be used to endorse or promote products derived
# from this software without specific prior written permission.
# NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Moddable modifications to allow building in the Moddable environment

sudo fuser -k 3333/tcp
openocd -f qca402x_openocd.cfg &                   
var="$(pgrep -f openocd)"                                                       
#echo "$var"  
cd output
mkdir -p gdbout

# ROOTDIR=$(cd ../../../../../..;pwd)
CURDIR=$(pwd)
ROOTDIR=$SDK
SCRIPTDIR=$ROOTDIR/build/tools/flash

# M4_DIR=${M4_DIR:-$ROOTDIR/quartz/demo/Helloworld_demo/build/gcc/output}
M4_DIR=${M4_DIR:-$CURDIR}
M4_IMAGE=${M4_IMAGE:-$M4_DIR/Quartz_HASHED.elf}
echo "M4_DIR: $M4_DIR Image: $M4_IMAGE"

M0_DIR=${M0_DIR:-$ROOTDIR/bin/cortex-m0/threadx}
M0_IMAGE=${M0_IMAGE:-$M0_DIR/ioe_ram_m0_threadx_ipt.mbn}

# Set WLAN_IMAGE=none (or any non-existent filename) to avoid programming a WLAN image.
WLAN_DIR=${WLAN_DIR:-$ROOTDIR/bin/wlan}
WLAN_IMAGE=${WLAN_IMAGE:-$WLAN_DIR/wlan_fw_img.bin}

GDB_PATH=${GDB_PATH:-`which arm-none-eabi-gdb`}
#GDB_SERVER_PATH=${GDB_SERVER_PATH:-`which JLinkGDBServer`}

GDB_SERVER=${GDB_SERVER:-localhost}
GDB_PORT=${GDB_PORT:-3333}

# Create partition_table.xml
if [ -e $WLAN_IMAGE ]
then
#    python $SCRIPTDIR/gen_part_table.py \
#	    --begin=3BLK \
#	    --file=$M4_IMAGE \
#	    --file=$M0_IMAGE \
#	    --file=$WLAN_IMAGE 
    python $SCRIPTDIR/gen_part_table.py \
	    --begin=140KB --partition --file=$M4_IMAGE \
        --partition --file=$M0_IMAGE \
        --partition --file=$WLAN_IMAGE \
        --partition --id=FS1 --size=64KB --start=12KB \
        --partition --id=FS2 --size=64KB --start=76KB \
        --partition --id=UNUSED --size=8KB --start=4KB               
else
#    python $SCRIPTDIR/gen_part_table.py \
#	    --begin=3BLK \
#	    --file=$M4_IMAGE \
#	    --file=$M0_IMAGE
    python $SCRIPTDIR/gen_part_table.py \
	    --begin=140KB --partition --file=$M4_IMAGE \
        --partition --file=$M0_IMAGE \
        --partition --id=FS1 --size=64KB --start=12KB \
        --partition --id=FS2 --size=64KB --start=76KB \
        --partition --id=UNUSED --size=8KB --start=4KB               
fi

if [ $? -ne 0 ]
then
    echo Abort flash.sh: gen_part_table.py failed
    exit 1
fi

# Convert to fwd_table.xml
python $SCRIPTDIR/gen_fwd_table.py \
	-x generated_partition_table.xml \
	--rawprogram generated_fwd_table.xml

if [ $? -ne 0 ]
then
    echo Abort flash.sh: gen_fwd_table.py failed
    exit 1
fi

python $SCRIPTDIR/flash_through_gdb.py \
	--rawprogram=generated_fwd_table.xml \
	--verbose=5 \
	--verify \
	--outputdir=gdbout \
	--gdbport=$GDB_PORT \
	--gdbpath=$GDB_PATH \
	--jtagprgelf=$SCRIPTDIR/JTAGPROGRAMMER_IMG_ARNTRI.elf \
	--search_path=$M4_DIR,$M0_DIR,$WLAN_DIR \
	--gdbserver $GDB_SERVER:$GDB_PORT

if [ $? -ne 0 ]
then
    echo Abort flash.sh: flash_through_gdb.py failed
	sleep 1
	kill $var
    exit 1
fi
echo " Finished Flashing"
sleep 2
kill $var
echo "Disconnected Openocd"

exit 0
