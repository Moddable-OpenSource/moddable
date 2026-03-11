#include "piuPebble.h"

static void PiuFontDelete(void* it);
static void PiuFontMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuFontListDelete(void* it);
static void PiuFontListMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static xsHostHooks PiuFontHooks = {
	PiuFontDelete,
	PiuFontMark,
	NULL
};

static const xsHostHooks PiuFontListHooks = {
	PiuFontListDelete,
	PiuFontListMark,
	NULL
};

void PiuFontDelete(void* it)
{
}

PiuDimension PiuFontGetAscent(PiuFont* self)
{
	return (*self)->ascent;
}

PiuDimension PiuFontGetHeight(PiuFont* self)
{
	return (*self)->height;
}

PiuDimension PiuFontGetWidth(PiuFont* self, xsSlot* string, xsIntegerValue offset, xsIntegerValue length)
{
	xsMachine* the = (*self)->the;
	xsStringValue text = PiuToString(string);
	PiuDimension width = 0;
	text += offset;
	if ((*self)->gfont) {
		char tmp[100];

		GContext *ctx = app_state_get_graphics_context();
		if ((-1 != length) && text[length]) {
			if (length >= (xsIntegerValue)sizeof(tmp))
				length = sizeof(tmp) - 1;

			c_memmove(tmp, text, length);
			tmp[length] = 0;
			text = tmp;
		}

		GSize size = graphics_text_layout_get_max_used_size(
					ctx, text,
				(*self)->gfont, GRect(0, 0, 10000, 100),
					GTextOverflowModeTrailingEllipsis,
					GTextAlignmentLeft, NULL);


		width = size.w;
	}
	else {
		xsIntegerValue character = 0;
		CFEGlyph glyph;
		CFESetFontData(gCFE, (*self)->buffer, (*self)->bufferSize);
		CFESetFontSize(gCFE, (*self)->size);
		while (length) {
			xsStringValue formerText = text;
	#if MODDEF_CFE_KERN
			xsIntegerValue formerCharacter = character;
	#endif
			text = fxUTF8Decode(text, &character);
			if (character <= 0)
				break;
			length -= text - formerText;
			glyph = CFEGetGlyphFromUnicode(gCFE, character, 0);
			if (!glyph) {
				character = '?';
				glyph = CFEGetGlyphFromUnicode(gCFE, character, 0);
				if (!glyph)
					continue;
			}
			width += glyph->advance;
	#if MODDEF_CFE_KERN
			if (formerCharacter)
				width += CFEGetKerningOffset(gCFE, formerCharacter, character);
	#endif
		}
	}
	return width;
}


void PiuFontMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuFont self = it;
	PiuMarkHandle(the, self->texture);
}

void PiuFontNew(xsMachine* the)
{
	PiuFont* self;
	xsResult = xsNewHostObject(NULL);
	xsSetHostChunk(xsResult, NULL, sizeof(PiuFontRecord));
	self = PIU(Font, xsResult);
	(*self)->reference = xsToReference(xsResult);
	(*self)->the = the;
	xsSetHostHooks(xsResult, &PiuFontHooks);
}

void PiuStyleLookupFont(PiuStyle* self)
{
	xsMachine* the = (*self)->the;
	void* archive = NULL;
	PiuFontList* fontList;
	PiuFont* font = NULL;
	xsStringValue name;
	char path[256];
	void* buffer;
	size_t bufferSize;
	int32_t ascent, descent, leading;
	
	if ((*self)->archive) {
		xsResult = xsReference((*self)->archive);
		archive = xsGetHostData(xsResult);
	}
	xsResult = xsGet(xsGlobal, xsID_fonts);
	if (xsTest(xsResult))
		fontList = PIU(FontList, xsResult);
	else {
		gCFE = CFENew();
		PiuFontListNew(the);
		xsSet(xsGlobal, xsID_fonts, xsResult);
		fontList = PIU(FontList, xsResult);
	}
	font = (*fontList)->first;
	while (font) {
		if (((*font)->family == (*self)->family)
				&& ((*font)->size == (*self)->size)
				&& ((*font)->weight == (*self)->weight)
				&& ((*font)->flags == ((*self)->flags & piuStyleBits))) {
			(*self)->font = font;
			return;
		}
		font = (*font)->next;
	}
	
	name = xsName((*self)->family);
	if (name)
		c_strcpy(path, name);
	else
		c_strcpy(path, "undefined");
	if (((*self)->flags & (piuStyleCondensedBit | piuStyleItalicBit)) || ((*self)->weight) || ((*self)->size)) {
		xsIntegerValue flag = 0;
		c_strcat(path, "-");
		if ((*self)->flags & piuStyleCondensedBit)
			c_strcat(path, "Condensed");
		else
			flag |= 1;
		switch ((*self)->weight) {
			case 1: c_strcat(path, "Ultralight"); break;
			case 2: c_strcat(path, "Thin"); break;
			case 3: c_strcat(path, "Light"); break;
			case 5: c_strcat(path, "Medium");break;
			case 6: c_strcat(path, "Semibold"); break;
			case 7: c_strcat(path, "Bold");break;
			case 8: c_strcat(path, "Heavy"); break;
			case 9: c_strcat(path, "Black");break;
			default: flag |= 2; break;
		}
		if ((*self)->flags & piuStyleItalicBit)
			c_strcat(path, "Italic"); 
		else
			flag |= 4;
		if (flag == 7)
			c_strcat(path, "Regular");
	}
	
	PiuFontNew(the);
	font = PIU(Font, xsResult);
	(*font)->gfont = modFindPebbleFont(path, (*self)->size, &ascent, &descent, &leading);
	if ((*font)->gfont == NULL) {
		if ((*self)->size) {
			xsIntegerValue length = c_strlen(path) + 1;
			c_strcat(path, "-");
			fxIntegerToString(NULL, (*self)->size, path + length, sizeof(path) - length);
		}
		name = path + c_strlen(path);
		c_strcpy(name, ".fnt");
		buffer = (uint8_t *)fxGetResource(the, archive, path, &bufferSize);
		if (!buffer)
			xsURIError("font not found: %s", path);
		(*font)->buffer = buffer;
		(*font)->bufferSize = bufferSize;
		CFESetFontData(gCFE, buffer, bufferSize);
		CFESetFontSize(gCFE, (*self)->size);
		CFEGetFontMetrics(gCFE, &ascent, &descent, &leading);
		
		(*font)->next = (*fontList)->first;
		(*fontList)->first = font;
			
		c_strcpy(name, ".png");
		xsResult = xsNewObject();
		if (archive)
			xsDefine(xsResult, xsID_archive, xsReference((*self)->archive), xsDefault);
		xsDefine(xsResult, xsID_path, xsString(path), xsDefault);
		xsResult = xsNew1(xsGlobal, xsID_Texture, xsResult);
		(*font)->texture = PIU(Texture, xsResult);
	}
	else {
		(*font)->next = (*fontList)->first;
		(*fontList)->first = font;
 	}
   
	(*font)->flags = (*self)->flags & piuStyleBits;
	(*font)->family = (*self)->family;
	(*font)->size = (*self)->size;
	(*font)->weight = (*self)->weight;
	
	(*font)->ascent = ascent;
	(*font)->height = ascent + descent + leading;
	
	(*self)->font = font;
}

void PiuFontListDelete(void* it)
{
	if (gCFE) {
		CFEDispose(gCFE);
		gCFE = C_NULL;
	}
}

void PiuFontListMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuFontList self = it;
	PiuFont* font = self->first;
	while (font) {
		PiuMarkHandle(the, font);
		font = (*font)->next;
	}
}

void PiuFontListNew(xsMachine* the)
{
	PiuFontList* self;
	xsResult = xsNewHostObject(NULL);
	xsSetHostChunk(xsResult, NULL, sizeof(PiuFontListRecord));
	self = PIU(FontList, xsResult);
	(*self)->reference = xsToReference(xsResult);
	xsSetHostHooks(xsResult, &PiuFontListHooks);
}
