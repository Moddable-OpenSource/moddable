/* Linker script to configure memory regions. */

SEARCH_DIR(.)
GROUP(-lgcc -lc -lnosys)

/*
  FLASH (rx) : ORIGIN = 0x26000, LENGTH = 0xce000
  RAM (rwx) :  ORIGIN = 0x20000000, LENGTH = 0x40000
*/
MEMORY
{
  FLASH (rx) : ORIGIN = 0x26000, LENGTH = 0xce000
  RAM (rwx) :  ORIGIN = 0x200057b8, LENGTH = 0x3a384
  NOINIT (rwx) :  ORIGIN = 0x2003fb3c, LENGTH = 0x100
}

/* much of Moddable code goes into flash */
/* this is in nrf_common.h */
/*  .rodata : { KEEP(*(.rodata.*)) } > FLASH */

SECTIONS
{
  . = ALIGN(4);

  .rodata.str : { KEEP(*(.rodata.str.1.4)) } > FLASH
  .xs6ro2 : { KEEP(*(.xs6ro*)) } > FLASH
  .irom.text : {
		KEEP(*(.irom.str))
		KEEP(*(.irom0.str.*))
		KEEP(*(.irom2.literal))
		KEEP(*(.irom2.text.literal))
		KEEP(*(.irom2.text))
		KEEP(*(.irom1.literal))
		KEEP(*(.irom1.text.literal))
		KEEP(*(.irom1.text))
	} > FLASH
	
} INSERT AFTER .data;

SECTIONS
{
  . = ALIGN(4);
  .mem_section_dummy_ram :
  {
  }
  .cli_sorted_cmd_ptrs :
  {
    PROVIDE(__start_cli_sorted_cmd_ptrs = .);
    KEEP(*(.cli_sorted_cmd_ptrs))
    PROVIDE(__stop_cli_sorted_cmd_ptrs = .);
  } > RAM
  .fs_data :
  {
    PROVIDE(__start_fs_data = .);
    KEEP(*(.fs_data))
    PROVIDE(__stop_fs_data = .);
  } > RAM
  .log_dynamic_data :
  {
    PROVIDE(__start_log_dynamic_data = .);
    KEEP(*(SORT(.log_dynamic_data*)))
    PROVIDE(__stop_log_dynamic_data = .);
  } > RAM
  .log_filter_data :
  {
    PROVIDE(__start_log_filter_data = .);
    KEEP(*(SORT(.log_filter_data*)))
    PROVIDE(__stop_log_filter_data = .);
  } > RAM
} INSERT AFTER .data;

SECTIONS
{
  .no_init (NOLOAD):
  {
    PROVIDE(__start_no_init_data = .);
	KEEP(*(SORT(.no_init*)))
	PROVIDE(__stop_no_init_data = .);
  } > NOINIT
}

SECTIONS
{
  .mem_section_dummy_rom :
  {
  }
  .sdh_soc_observers :
  {
    PROVIDE(__start_sdh_soc_observers = .);
    KEEP(*(SORT(.sdh_soc_observers*)))
    PROVIDE(__stop_sdh_soc_observers = .);
  } > FLASH
 .sdh_ble_observers :
  {
    PROVIDE(__start_sdh_ble_observers = .);
    KEEP(*(SORT(.sdh_ble_observers*)))
    PROVIDE(__stop_sdh_ble_observers = .);
  } > FLASH
  .sdh_req_observers :
  {
    PROVIDE(__start_sdh_req_observers = .);
    KEEP(*(SORT(.sdh_req_observers*)))
    PROVIDE(__stop_sdh_req_observers = .);
  } > FLASH
  .sdh_state_observers :
  { 
    PROVIDE(__start_sdh_state_observers = .);
    KEEP(*(SORT(.sdh_state_observers*)))
    PROVIDE(__stop_sdh_state_observers = .);
  } > FLASH
  .sdh_stack_observers :
  { 
    PROVIDE(__start_sdh_stack_observers = .);
    KEEP(*(SORT(.sdh_stack_observers*)))
    PROVIDE(__stop_sdh_stack_observers = .);
  } > FLASH
  .cli_command :
  {
    PROVIDE(__start_cli_command = .);
    KEEP(*(.cli_command))
    PROVIDE(__stop_cli_command = .);
  } > FLASH
  .crypto_data :
  {
    PROVIDE(__start_crypto_data = .);
    KEEP(*(SORT(.crypto_data*)))
    PROVIDE(__stop_crypto_data = .);
  } > FLASH
  .pwr_mgmt_data :
  {
    PROVIDE(__start_pwr_mgmt_data = .);
    KEEP(*(SORT(.pwr_mgmt_data*)))
    PROVIDE(__stop_pwr_mgmt_data = .);
  } > FLASH
  .nrf_queue :
  {
    PROVIDE(__start_nrf_queue = .);
    KEEP(*(.nrf_queue))
    PROVIDE(__stop_nrf_queue = .);
  } > FLASH
  .log_const_data :
  {
    PROVIDE(__start_log_const_data = .);
    KEEP(*(SORT(.log_const_data*)))
    PROVIDE(__stop_log_const_data = .);
  } > FLASH
  .log_backends :
  {
    PROVIDE(__start_log_backends = .);
    KEEP(*(SORT(.log_backends*)))
    PROVIDE(__stop_log_backends = .);
  } > FLASH
  .nrf_balloc :
  {
    PROVIDE(__start_nrf_balloc = .);
    KEEP(*(.nrf_balloc))
    PROVIDE(__stop_nrf_balloc = .);
  } > FLASH
  .xs_lib_flash :
  {
    PROVIDE(__start_xs_lib = .);
    KEEP(*(SORT(.flash*)))
    PROVIDE(__stop_xs_lib = .);
  } > FLASH

} INSERT AFTER .text

/* located in the nRF52 sdk at modules/nrfx/mdk/nrf_common.ld */

INCLUDE "nrf_common.ld"
