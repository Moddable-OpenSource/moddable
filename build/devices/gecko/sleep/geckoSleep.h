#include <stdint.h>

uint32_t geckoGetPersistentValue(uint32_t reg);
void geckoSetPersistentValue(uint32_t reg, uint32_t val);
void geckoSleepEM4(uint32_t ms);

