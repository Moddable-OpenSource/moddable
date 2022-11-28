#include "xsmc.h"
#include "esp_system.h"

void xs_esp32_reset_now(xsMachine *the)
{
	esp_restart();

	while (1)
		modDelayMilliseconds(1000);
}