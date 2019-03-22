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


define runm0
monitor jtagconf 0 0
monitor sleep 100
# EACH time to reset and start over
set confirm no
set *((int *)0xe000ed0c)=0x05fa0006
monitor reset
monitor sleep 1000


#Clear out CLM 
restore zero.bin binary 0x5000000
restore zero.bin binary 0x5008000
restore zero.bin binary 0x5010000
restore zero.bin binary 0x5018000

if $COMBINED_BIN == 1

   restore ../../../../../core/bsp/std_scripts/M0_ROM/m0_rom.bin binary 0 0
else

   #Load PBL binary
   restore ../../../../../bin/cortex-m0/threadx/m0_pbl.bin binary 0 0

   #Load IPT version of ROM elf
   load ../../../../../bin/cortex-m0/threadx/IOE_ROM_IPT_IMG_BRNTRI.elf

end

#Load IPT version of RAM symbols
symbol-file ../../../../../bin/cortex-m0/threadx/IOE_RAM_IPT_IMG_BRNTRI.elf

# Load symbols (only needed for debugging)
add-symbol-file ../../../../../bin/cortex-m0/threadx/IOE_ROM_IPT_IMG_BRNTRI_patched_syms.elf 0x0000C000 

if $COMBINED_BIN == 0
# GCC appears to load initialized data into the execution region and not the load region
# since the scatter loader for Realview copies initialized data from the load region
# to the execution region do the opposite copy here such that the scatter loader
# doesn't clear initialized data (by copying from the un-initialized load region)
if *((unsigned long *)&Image__FMM_MISSIONROM_RW_REGION_Length)
  p *((unsigned long *)&Image__FMM_MISSIONROM_RW_REGION_Base)
  p *((unsigned long *)&Image__FMM_MISSIONROM_RW_REGION_Base) + *((unsigned long *)&Image__FMM_MISSIONROM_RW_REGION_Length)
  dump memory output/gdb_temp_m0 $$1 $$0
  p *((unsigned long *)&Load__FMM_MISSIONROM_RW_REGION_Base)
  restore output/gdb_temp_m0 binary $0
end

if *((unsigned long *)&Image__LMM_MISSIONROM_RW_REGION_Length)
  p *((unsigned long *)&Image__LMM_MISSIONROM_RW_REGION_Base)
  p *((unsigned long *)&Image__LMM_MISSIONROM_RW_REGION_Base) + *((unsigned long *)&Image__LMM_MISSIONROM_RW_REGION_Length)
  dump memory output/gdb_temp_m0 $$1 $$0
  p *((unsigned long *)&Load__LMM_MISSIONROM_RW_REGION_Base)
  restore output/gdb_temp_m0 binary $0
end
end

set  disassemble-next-line on
show disassemble-next-line

#Set PC and SP
#set $sp=0x05017c00
set $pc=0x1A0

b *0x3cf0


b ROM_init

#Enable mock-sleep: JTAG domain stays powered on
set *((int*)0x40B00048)=0x7
set *((int*)0x40B0003C)=0x7 
#set *((int *)0xe000ed0c)=0x05fa0002
flushregs
set confirm yes
c

end


# ONE-TIME when starting gdb
target extended-remote localhost:2330
monitor speed auto
monitor endian little

define hookpost-stepi
x/1i $pc
end
set history filename output/gdb_history_m0.log
set history save on
set print pretty on
set print object on
set print vtbl on
set pagination off
set output-radix 16
set disassemble-next-line on
source v1/v1_substitute_path_m0.gdb
set $COMBINED_BIN=0
runm0
