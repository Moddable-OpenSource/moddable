/* Flash Split for 4M chips with two application slots for OTA update (esptool flash size 4MB-c1) */
/* slot 1 1020KB at 0x001000: sketch and mods */
/* slot 2 1020KB at 0x101000: sketch and mods */
/* preferences 8 KB */
/* spiffs 2020KB */
/* eeprom 20KB */

MEMORY
{
  dport0_0_seg :                        org = 0x3FF00000, len = 0x10
  dram0_0_seg :                         org = 0x3FFE8000, len = 0x14000
  iram1_0_seg :                         org = 0x40100000, len = 0x8000
  irom0_0_seg :                         org = 0x40201010, len = 0xfeff0
}

PROVIDE ( _MODPREF_start = 0x40400000 );
PROVIDE ( _MODPREF_end = 0x40402000 );
PROVIDE ( _SPIFFS_start = 0x40402000 );
PROVIDE ( _SPIFFS_end = 0x405FB000 );
PROVIDE ( _SPIFFS_page = 0x100 );
PROVIDE ( _SPIFFS_block = 0x2000 );

INCLUDE "eagle.app.v6.common.ld"
