#include "piuAll.h"

typedef struct PiuProfileStruct PiuProfileRecord, *PiuProfile;

struct PiuProfileStruct {
	PiuHandlePart;
	xsNumberValue propagation;
	xsIntegerValue length;
	PiuProfile* callers[1];
};

extern void PiuProfileDelete(void* it);
static void PiuProfileMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuProfile_propagateAux(xsMachine* the, PiuProfile* self, xsNumberValue duration, PiuProfile* callee);

const xsHostHooks ICACHE_FLASH_ATTR PiuProfileHooks = {
	PiuProfileDelete,
	PiuProfileMark,
	NULL
};

void PiuProfileDelete(void* it)
{
}

void PiuProfileMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
// 	PiuProfile self = it;
}

void PiuProfile_createCache(xsMachine* the)
{
	xsIntegerValue length = xsToInteger(xsArg(0));
	xsIntegerValue size = sizeof(PiuProfileRecord) + (sizeof(PiuProfile*) * length);
	PiuProfile* self;
	xsSetHostChunk(xsThis, NULL, size);
	self = PIU(Profile, xsThis);
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuProfileHooks);
	(*self)->propagation = 0;
	(*self)->length = length;
}

void PiuProfile_deleteCache(xsMachine* the)
{
	PiuProfile* self = PIU(Profile, xsThis);
	xsSet(xsThis, xsID_propagation, xsNumber((*self)->propagation));
	xsSetHostData(xsThis, NULL);
}

void PiuProfile_fillCache(xsMachine* the)
{
	PiuProfile* self = PIU(Profile, xsThis);
	PiuProfile* caller = PIU(Profile, xsArg(0));
	xsIntegerValue index = xsToInteger(xsArg(1));
	(*self)->callers[index] = caller;
}

void PiuProfile_propagate(xsMachine* the)
{
	PiuProfile* self = PIU(Profile, xsThis);
	xsNumberValue duration = xsToNumber(xsArg(0));
	xsVars(2);
	PiuProfile_propagateAux(the, self, duration, C_NULL);
}

void PiuProfile_propagateAux(xsMachine* the, PiuProfile* self, xsNumberValue duration, PiuProfile* callee)
{
	xsIntegerValue length = (*self)->length, index;
	if ((*self)->callers[length]) {
		xsVar(0) = xsReference((*self)->reference);
		xsVar(1) = xsReference((*callee)->reference);
		xsCall1(xsVar(0), xsID_removeCallee, xsVar(1));	
		xsCall1(xsVar(1), xsID_removeCaller, xsVar(0));	
		return;
	}
	(*self)->propagation += duration;
	if (length) {
		duration /= length;
		if (duration >= 1.0) {
			(*self)->callers[length] = self;
			for (index = 0; index < length; index++) {
				PiuProfile_propagateAux(the, (*self)->callers[index], duration, self);
			}
			(*self)->callers[length] = C_NULL;
		}
	}
}