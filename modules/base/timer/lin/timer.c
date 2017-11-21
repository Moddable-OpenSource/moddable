#include "mc.xs.h"

typedef struct {
	xsMachine *the;
	xsSlot slot;
	xsSlot callback;
	xsIntegerValue interval;
	xsIntegerValue repeat;
	guint g_timer;
} ModTimerRecord, *ModTimer;

static gboolean ModTimerCallback(gpointer data);
static ModTimer ModTimerCreate(xsMachine* the);
static void ModTimerDelete(void* it);

gboolean ModTimerCallback(gpointer data)
{
	ModTimer self = data;
	xsBeginHost(self->the);
	xsVars(2);
	xsVar(0) = xsAccess(self->callback);
	xsVar(1) = xsAccess(self->slot);
	xsCallFunction1(xsVar(0), xsGlobal, xsVar(1));
	xsEndHost(self->the);
	if (self->repeat) {
		if (self->interval != self->repeat) {
			self->interval = self->repeat;
			self->g_timer = g_timeout_add(self->interval, ModTimerCallback, self);
			return G_SOURCE_REMOVE;
		}
		return G_SOURCE_CONTINUE;
	}
	self->g_timer = 0;
	return G_SOURCE_REMOVE;
}

ModTimer ModTimerCreate(xsMachine* the)
{
	ModTimer self = c_malloc(sizeof(ModTimerRecord));
	xsResult = xsNewHostObject(ModTimerDelete);
	xsSetHostData(xsResult, self);
	self->the = the;
	self->slot = xsResult;
	self->callback = xsArg(0);
	xsRemember(self->slot);
	xsRemember(self->callback);
	return self;
}

void ModTimerDelete(void* it)
{
	ModTimer self = it;
	guint g_timer = self->g_timer;
	if (g_timer) {
		g_source_remove(g_timer);
		self->g_timer = 0;
	}
}

void xs_timer_set(xsMachine *the)
{
	int argc = xsToInteger(xsArgc);
	ModTimer self = ModTimerCreate(the);
	self->interval = (argc > 1) ? xsToInteger(xsArg(1)) : 0;
	self->repeat = (argc > 2) ? xsToInteger(xsArg(2)) : 0;
	self->g_timer = g_timeout_add(self->interval, ModTimerCallback, self);
}

void xs_timer_repeat(xsMachine *the)
{
	ModTimer self = ModTimerCreate(the);
	self->interval = self->repeat = xsToInteger(xsArg(1));
	self->g_timer = g_timeout_add(self->interval, ModTimerCallback, self);
}

void xs_timer_schedule(xsMachine *the)
{
	int argc = xsToInteger(xsArgc);
	ModTimer self = (ModTimer)xsGetHostHandle(xsArg(0));
	ModTimerDelete(self);
	self->interval = xsToInteger(xsArg(1));
	self->repeat = (argc > 2) ? xsToInteger(xsArg(2)) : 0;
	self->g_timer = g_timeout_add(self->interval, ModTimerCallback, self);
}

void xs_timer_clear(xsMachine *the)
{
	ModTimer self = (ModTimer)xsGetHostHandle(xsArg(0));
	xsForget(self->callback);
	xsForget(self->slot);
	ModTimerDelete(self);
}

void xs_timer_delay(xsMachine *the)
{
	usleep(xsToInteger(xsArg(0)) * 1000);
}

