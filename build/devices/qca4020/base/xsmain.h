
#include <stdio.h>
#include "qapi_types.h"
#include "qurt_error.h"
#include "qurt_signal.h"

#define kSIG_WAKE_MAIN			0x00000001
#define kSIG_THREAD_CREATED		0x00000002
#define kSIG_SERVICE_DEBUGGER	0x00000004

extern qurt_signal_t gMainSignal;

