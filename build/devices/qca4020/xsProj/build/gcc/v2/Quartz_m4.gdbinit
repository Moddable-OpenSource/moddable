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



define runm4

#Bring M0 out of reset
monitor jtagconf 0 0
set *((int *)0xe000ed0c)=0x05fa0006
monitor reset

set *((int *)0x50742090)=0
set *((int *)0x50803300)=0 
set *((int *)0x50803308)=1
set *((int *)0x50310000)=0x7

monitor sleep 1000

monitor jtagconf 4 1

set *((int *)0x50742090)=0
monitor sleep 100

p *((int *)0x50310000)


# EACH time to reset and start over
set confirm no
set *((int *)0xe000ed0c)=0x05fa0003
monitor reset

set *((int *)0x100005ec)=0x200


#Disable Clock gating for M4 Crypto to work: TODO - remove when Crypto fix is available
set *((int *)0x50680028)=1

# (MEMSS_CPUCRITICAL bit)- activate XIP
set *((int *)0x48008000)=0

#Enable writing to ROM
set *((int *)0x50680020)=1

#Clear RTC TImer enable bit
#set *((int *)0x50742074)=0 

#(Enable OTP clock)
set *((int *)0x50803418)=1

#(Power up the OTP) 
set *((int *)0x50741800)=1 

#(Write blow time of 10usec)
set *((int *)0x50741804)=0x78

#(time required for successive fuse blow) 
set *((int *)0x50741808)=0xC

#(M4 to boot from ROM)
set *((int *)0x507401BC)=0x2

#(M0 to boot from ROM)
set *((int *)0x507401BC)=1 

#(Set M0 out of reset option)
set *((int *)0x507401BC)=0x10

#Initialize hash in OTP
if $SECBOOT == 1
   source inithash.gdb
end   

#(Reset OTP)
set *((int *)0x50803410)=1

#wait for some time (1 sec)
monitor sleep 1000

#(Reset OTP)
set *((int *)0x50803410)=0 


restore zero.bin binary 0x10000000

if $COMBINED_BIN == 1

   #Load PBL binary
   restore ../../../../../core/bsp/std_scripts/M4_ROM/m4_rom.bin binary 0 0

else

   restore ../../../../../bin/cortex-m4/m4f_pbl.bin binary 0 0

   #Load ROM elfs
   load ../../../../../bin/cortex-m4/IOE_ROM_IPT_IMG_ARNNRI_compact.elf

end

#Load symbols
symbol-file output/Quartz.elf

add-symbol-file ../../../../../bin/cortex-m4/IOE_ROM_IPT_IMG_ARNNRI_patched_syms.elf 0x10000

if $COMBINED_BIN == 0
# START HACK {
# There's no proper ROM image yet, but ROM_data_init attempts to copy
# initialized data from ROM to RAM for SOM and FOM Operational Modes
# and for the Vector Table.  Do the OPPOSITE copy here in preparation
# so that ROM_data_init will work.
if *((unsigned long *)&Image__RAM_RW_MOM_Length)
  p *((unsigned long *)&Image__RAM_RW_MOM_Base)
  p *((unsigned long *)&Image__RAM_RW_MOM_Base) + *((unsigned long *)&Image__RAM_RW_MOM_Length)
  dump memory output/gdb_temp $$1 $$0
  p *((unsigned long *)&Load__RAM_RW_MOM_Base)
  restore output/gdb_temp binary $0
end
if *((unsigned long *)&Image__RAM_RW_SOM_Length)
  p *((unsigned long *)&Image__RAM_RW_SOM_Base)
  p *((unsigned long *)&Image__RAM_RW_SOM_Base) + *((unsigned long *)&Image__RAM_RW_SOM_Length)
  dump memory output/gdb_temp $$1 $$0
  p *((unsigned long *)&Load__RAM_RW_SOM_Base)
  restore output/gdb_temp binary $0
end
if *((unsigned long *)&Image__RAM_RW_FOM_Length)
  p *((unsigned long *)&Image__RAM_RW_FOM_Base)
  p *((unsigned long *)&Image__RAM_RW_FOM_Base) + *((unsigned long *)&Image__RAM_RW_FOM_Length)
  dump memory output/gdb_temp $$1 $$0
  p *((unsigned long *)&Load__RAM_RW_FOM_Base)
  restore output/gdb_temp binary $0
end
# if *((unsigned long *)&Image__VECTOR_TABLE_Length)
#   p *((unsigned long *)&Image__VECTOR_TABLE_Base)
#   p *((unsigned long *)&Image__VECTOR_TABLE_Base) + *((unsigned long *)&Image__VECTOR_TABLE_Length)
#   dump memory output/gdb_temp $$1 $$0
#   p *((unsigned long *)&Load__VECTOR_TABLE_Base)
#   restore output/gdb_temp binary $0
# end
# END HACK }
end


#Set PC and SP
#set $sp=Image$$RAM_FOM_BSP_STACK_ZI_REGION$$ZI$$Limit
set $sp=0x10002400


# SET PC to start of PBL
set $pc=0x1C0


set *((int *)0xe000ed0c)=0x05fa0002
set *((int *)0xe000e010)=0x00010003
set *((int *)0xe000ed04)=0x02000000
flushregs
set confirm yes

#break in PBL - wait for M0 to come up

b *0x6dd8
#b *0x65c8
set *(int*)0x5021D004=0
b sbl1_main_ctl

#Set the cookie to allow boot past SBL
set *(int*)0x10000484=0xDADBEDAD

c

end


# ONE-TIME when starting gdb

target extended-remote localhost:2331
#target extended-remote ubuntu-dsl04:3333
monitor speed auto
monitor endian little

define hookpost-stepi
x/1i $pc
end
set history filename output/gdb_history.log
set history save on
set print pretty on
set print object on
set print vtbl on
set pagination off
set output-radix 16
set $SECBOOT=0
set $COMBINED_BIN=0
runm4
