/* Flash Split for !M chips */
/* sketch 1019KB */
/* spiffs 3052KB */
/* eeprom 20KB */

MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = 0x40201010, len = 0xf9ff0
}

PROVIDE ( _XSMOD_start = 0 );
PROVIDE ( _XSMOD_end = 0 );
PROVIDE ( _MODPREF_start = 0x402FB000 );
PROVIDE ( _MODPREF_end = 0x402FD000 );
PROVIDE ( _SPIFFS_start = 0x402FB000 );
PROVIDE ( _SPIFFS_end = 0x402FB000 );
PROVIDE ( _SPIFFS_page = 0 );
PROVIDE ( _SPIFFS_block = 0 );

INCLUDE "eagle.app.v6.common.ld"
