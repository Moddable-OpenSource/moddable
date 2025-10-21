#include "piuPebble.h"

typedef struct PiuRoundRectStruct PiuRoundRectRecord, *PiuRoundRect;
struct PiuRoundRectStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	xsIntegerValue corners;
	xsIntegerValue radius;
	int fillColor;
};

static void PiuRoundRectDictionary(xsMachine* the, void* it);
static void PiuRoundRectDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuRoundRectDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuRoundRectDispatchRecord = {
	"RoundRect",
	PiuContentBind,
	PiuContentCascade,
	PiuRoundRectDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuContentInvalidate,
	PiuContentMeasureHorizontally,
	PiuContentMeasureVertically,
	PiuContentPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuContentSync,
	PiuContentUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuRoundRectHooks = {
	PiuContentDelete,
	PiuContentMark,
	NULL
};

void PiuRoundRectDictionary(xsMachine* the, void* it) 
{
	PiuRoundRect* self = it;
	xsIntegerValue integer;
	if (xsFindInteger(xsArg(1), xsID_corners, &integer)) {
		(*self)->corners = integer;
	}
	if (xsFindInteger(xsArg(1), xsID_radius, &integer)) {
		(*self)->radius = integer;
	}
}

void PiuRoundRectDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuRoundRect* self = it;
	PiuSkin* skin = (*self)->skin;
	if (skin) {
		PiuColorRecord color;
		PiuState state = (*self)->state;
		if (state < 0) state = 0;
		else if (3 < state) state = 3;
		PiuColorsBlend((*skin)->data.color.fill, state, &color);
		(*self)->fillColor = GColorFromRGBA(color.r, color.g, color.b, color.a).argb;
		PiuRectangleRecord bounds;
		PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
		PiuViewDrawContent(view, PiuRoundRectDrawAux, it, 0, 0, bounds.width, bounds.height);
	}
}

extern GContext *getPocoPebbleGContext(Poco poco);

void PiuRoundRectDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh)
{
	PiuRoundRect* self = it;
	GContext *ctx = (*view)->ctx;
	GRect r = GRect( x, y, sw, sh );
	int color = (*self)->fillColor;
	int radius = (*self)->radius;
	int corners = (*self)->corners;
	GColor saveColor = ctx->draw_state.fill_color;
	ctx->draw_state.fill_color.argb = color;
	graphics_fill_round_rect(ctx, &r, radius, corners);
	ctx->draw_state.fill_color = saveColor;
}

void PiuRoundRect_create(xsMachine* the)
{
	PiuRoundRect* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuRoundRectRecord));
	self = PIU(RoundRect, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuRoundRectHooks);
	(*self)->dispatch = (PiuDispatch)&PiuRoundRectDispatchRecord;
	(*self)->recordSize = PiuRecordSize(sizeof(PiuRoundRectRecord));
	(*self)->flags = piuVisible;
	(*self)->corners = 0xF;
	(*self)->radius = 8;
	PiuContentDictionary(the, self);
	PiuRoundRectDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuRoundRect_get_corners(xsMachine* the)
{
	PiuRoundRect* self = PIU(RoundRect, xsThis);
	xsResult = xsInteger((*self)->corners);
}

void PiuRoundRect_set_corners(xsMachine* the)
{
	PiuRoundRect* self = PIU(RoundRect, xsThis);
	(*self)->corners = xsToInteger(xsArg(0));
	PiuContentInvalidate(self, NULL);
}

void PiuRoundRect_get_radius(xsMachine* the)
{
	PiuRoundRect* self = PIU(RoundRect, xsThis);
	xsResult = xsInteger((*self)->radius);
}

void PiuRoundRect_set_radius(xsMachine* the)
{
	PiuRoundRect* self = PIU(RoundRect, xsThis);
	(*self)->radius = xsToInteger(xsArg(0));
	PiuContentInvalidate(self, NULL);
}
