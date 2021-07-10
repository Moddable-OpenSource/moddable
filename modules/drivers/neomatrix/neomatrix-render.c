#include "xsmc.h"
#include "mc.defines.h"

#ifndef MODDEF_NEOMATRIX_PIN
  #define MODDEF_NEOMATRIX_PIN (21)
#endif

#ifndef MODDEF_NEOMATRIX_WIDTH
  #define MODDEF_NEOMATRIX_WIDTH (16)
#endif

#ifndef MODDEF_NEOMATRIX_HEIGHT
  #define MODDEF_NEOMATRIX_HEIGHT (16)
#endif

#ifndef MODDEF_NEOMATRIX_BRIGHTNESS
  #define MODDEF_NEOMATRIX_BRIGHTNESS (32)
#endif

void xs_NeoMatrix_get_configure_pin(xsMachine *the) {
  xsmcSetInteger(xsResult, MODDEF_NEOMATRIX_PIN);
}

void xs_NeoMatrix_get_configure_width(xsMachine *the) {
  xsmcSetInteger(xsResult, MODDEF_NEOMATRIX_WIDTH);
}

void xs_NeoMatrix_get_configure_height(xsMachine *the) {
  xsmcSetInteger(xsResult, MODDEF_NEOMATRIX_HEIGHT);
}

void xs_NeoMatrix_get_configure_brightness(xsMachine *the) {
  xsmcSetInteger(xsResult, MODDEF_NEOMATRIX_BRIGHTNESS);
}
