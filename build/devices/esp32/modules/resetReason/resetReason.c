#include "xsmc.h"
#include "esp_system.h"

void xs_esp32_reset_reason(xsMachine *the)
{
    esp_reset_reason_t reason = esp_reset_reason();
    xsmcSetInteger(xsResult, reason);
}