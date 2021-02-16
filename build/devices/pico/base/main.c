#include <stdio.h>
#include "pico/stdlib.h"
#include "xsmain.h"

int main() {
	stdio_uart_init();

	xs_setup();
}

