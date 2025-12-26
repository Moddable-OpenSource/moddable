#include "piuPebble.h"

static void PiuTextureMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static xsHostHooks PiuTextureHooks = {
	PiuTextureDelete,
	PiuTextureMark,
	NULL
};

void PiuTextureCreate(xsMachine* the) 
{
	PiuTextureRecord record;
	PiuTexture self = &record;
	c_memset(self, 0, sizeof(record));
	self->reference = xsToReference(xsThis);
	if (xsTypeOf(xsArg(0)) == xsIntegerType) {
		int id = xsToInteger(xsArg(0));
		GBitmap *bitmap = gbitmap_create_with_resource(id);
		if (!bitmap)
			xsUnknownError("texture not found");
		self->gbitmap = bitmap;
		self->width = bitmap->bounds.size.w;
		self->height = bitmap->bounds.size.h;
	}
	else {
		if (xsTest(xsArg(0))) {
			CommodettoBitmap cb = xsGetHostChunk(xsArg(0));
			if (cb->format != kCommodettoBitmapMonochromeAligned)
				xsUnknownError("invalid texture format");
			self->flags |= piuTextureAlpha;
			self->mask.addr = cb->bits.data;
			self->mask.row_size_bytes = ((cb->w + 31) >> 5) * 4;
			self->mask.info.format = GBitmapFormat1Bit;
			self->mask.info.version = GBITMAP_VERSION_1;
			self->mask.bounds = GRect(0, 0, cb->w, cb->h);
			self->width = cb->w;
			self->height = cb->h;
		}
		if (xsTest(xsArg(1))) {
			CommodettoBitmap cb = xsGetHostChunk(xsArg(1));
			if (cb->format != kCommodettoBitmapMonochromeAligned)
				xsUnknownError("invalid texture format");
			self->flags |= piuTextureColor;
			self->bits.addr = cb->bits.data;
			self->bits.row_size_bytes = ((cb->w + 31) >> 5) * 4;
			self->bits.info.format = GBitmapFormat1Bit;
			self->bits.info.version = GBITMAP_VERSION_1;
			self->bits.bounds = GRect(0, 0, cb->w, cb->h);
			self->width = cb->w;
			self->height = cb->h;
		}
	}
	xsSetHostChunk(xsThis, self, sizeof(record));
	xsSetHostHooks(xsThis, &PiuTextureHooks);
	xsResult = xsThis;
}

void PiuTextureDelete(void* it)
{
	if (it) {
		PiuTexture self = it;
		if (self->gbitmap) {
			gbitmap_destroy(self->gbitmap);
			self->gbitmap = C_NULL;
		}
	}
}

void PiuTextureMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
}

void PiuTexture_get_width(xsMachine* the)
{
	PiuTexture* self = PIU(Texture, xsThis);
	xsResult = xsPiuDimension((*self)->width);
}

void PiuTexture_get_height(xsMachine* the)
{
	PiuTexture* self = PIU(Texture, xsThis);
	xsResult = xsPiuDimension((*self)->height);
}
