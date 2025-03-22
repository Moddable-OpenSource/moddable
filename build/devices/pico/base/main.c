#include <stdio.h>
#include "pico/stdlib.h"
#include "xsmain.h"

#if SFE_ALLOC
#include "sparkfun_pico/sfe_pico.h"
#endif

int main() {
	stdio_init_all();
//	set_sys_clock_khz(48000 * 5, true);

#if SFE_ALLOC
	size_t psram_size = sfe_setup_psram(SFE_RP2350_XIP_CSI_PIN);
	sfe_pico_alloc_init();
#endif

	xs_setup();
}

