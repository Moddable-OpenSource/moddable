#include <stdio.h>
#include "pico/stdlib.h"
#include "xsmain.h"

int main() {
	stdio_init_all();
	printf("start 'em up\n");

	xs_setup();
}

