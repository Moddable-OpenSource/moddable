#include "piuPebble.h"

typedef struct PiuInverterStruct PiuInverterRecord, *PiuInverter;
struct PiuInverterStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
};

static void PiuInverterDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuInverterDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuInverterDispatchRecord = {
	"Inverter",
	PiuContentBind,
	PiuContentCascade,
	PiuInverterDraw,
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

const xsHostHooks ICACHE_FLASH_ATTR PiuInverterHooks = {
	PiuContentDelete,
	PiuContentMark,
	NULL
};

void PiuInverterDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuInverter* self = it;
	PiuRectangleRecord bounds;
	PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
	PiuViewDrawContent(view, PiuInverterDrawAux, it, 0, 0, bounds.width, bounds.height);
}

extern GContext *getPocoPebbleGContext(Poco poco);

void PiuInverterDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh)
{
	GContext *ctx = (*view)->ctx;
	GBitmap* context_bitmap = graphics_context_get_bitmap(ctx);
	GBitmap sub_bitmap;
	x += ctx->draw_state.drawing_box.origin.x;
	y += ctx->draw_state.drawing_box.origin.y;
	GRect box = GRect(x, y, sw, sh);
  	gbitmap_init_as_sub_bitmap(&sub_bitmap, context_bitmap, box);
	GRect rect = sub_bitmap.bounds;
	rect.origin.x -= ctx->draw_state.drawing_box.origin.x;
	rect.origin.y -= ctx->draw_state.drawing_box.origin.y;
	GCompOp saveMode = ctx->draw_state.compositing_mode;
	graphics_context_set_compositing_mode(ctx, GCompOpAssignInverted);
	graphics_draw_bitmap_in_rect(ctx, &sub_bitmap, &rect);
	ctx->draw_state.compositing_mode = saveMode;
}

void PiuInverter_create(xsMachine* the)
{
	PiuInverter* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuInverterRecord));
	self = PIU(Inverter, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuInverterHooks);
	(*self)->dispatch = (PiuDispatch)&PiuInverterDispatchRecord;
	(*self)->recordSize = PiuRecordSize(sizeof(PiuInverterRecord));
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuBehaviorOnCreate(self);
}


