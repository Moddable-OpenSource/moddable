#ifndef __BUILTINCOMMON_H__
#define __BUILTINCOMMON_H__

uint8_t builtinArePinsFree(uint32_t pin);
uint8_t builtinUsePins(uint32_t pin);
void builtinFreePins(uint32_t pin);

uint8_t builtinHasCallback(xsMachine *the, xsIndex id);
uint8_t builtinGetCallback(xsMachine *the, xsIndex id, xsSlot *slot);

#define builtinCriticalSectionBegin() xt_rsil(0)
#define builtinCriticalSectionEnd() xt_rsil(15)

enum {
	kIOFormatByte = 1,
	kIOFormatBuffer = 2,

	kIOFormatNext,
};

void builtinGetFormat(xsMachine *the, uint8_t format);
uint8_t builtinSetFormat(xsMachine *the);

#endif
