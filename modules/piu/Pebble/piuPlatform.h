#include "modInstrumentation.h"

typedef int16_t PiuCoordinate;
typedef uint16_t PiuDimension;
#define xsPiuCoordinate(VALUE) xsInteger((xsIntegerValue)(VALUE))
#define xsPiuDimension(VALUE) xsInteger((xsIntegerValue)(VALUE))
#define xsToPiuCoordinate(SLOT) (PiuCoordinate)xsToInteger(SLOT)
#define xsToPiuDimension(SLOT) (PiuDimension)xsToInteger(SLOT)
