#include <stdio.h>
#include "pico/stdlib.h"
#include "xsmain.h"

int main() {
	stdio_init_all();
//	set_sys_clock_khz(48000 * 5, true);

	xs_setup();
}

