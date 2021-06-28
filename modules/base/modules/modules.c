#include "xsAll.h"
#include "xs.h"

extern void* xsPreparationAndCreation(xsCreation **creation);

void xs_modules_host(xsMachine *the)
{
	txPreparation *preparation = xsPreparationAndCreation(NULL);
	txInteger scriptCount = preparation->scriptCount, i = 0;
	txScript* script = preparation->scripts;

	xsResult = xsNewArray(0);

	while (scriptCount--) {
		char path[C_PATH_MAX];
		char *dot;

		c_strcpy(path, script->path);
		dot = c_strchr(path, '.');
		if (dot)
			*dot = 0;

		xsSetIndex(xsResult, i++, xsString(path));

		script++;
	}
}

#define mxArchiveHeaderSize (sizeof(Atom) + sizeof(Atom) + XS_VERSION_SIZE + sizeof(Atom) + XS_DIGEST_SIZE + sizeof(Atom) + XS_DIGEST_SIZE)

void xs_modules_archive(xsMachine *the)
{
	uint8_t *p, *q;
	int i = 0;
	char path[128];
	txPreparation *preparation;

	xsResult = xsNewArray(0);

	preparation = xsPreparationAndCreation(NULL);
	if (!preparation)
		return;
	c_memcpy(path, preparation->base, preparation->baseLength);

	p = the->archive;
	if (!p) return;

	p += mxArchiveHeaderSize;
	// NAME
	p += c_read32be(p);
	// SYMB
	p += c_read32be(p);
	// MODS
	q = p + c_read32be(p);
	p += sizeof(Atom);

	while (p < q) {
		int atomSize = c_read32be(p);

		c_strcpy(path + preparation->baseLength, (txString)(p + sizeof(Atom)));
		path[atomSize - sizeof(Atom) - 4] = 0;

		xsSetIndex(xsResult, i++, xsString(path + preparation->baseLength));

		p += atomSize;
		p += c_read32be(p);
	}
}

void xs_modules_importNow(xsMachine *the)
{
	char path[C_PATH_MAX];

	xsToStringBuffer(xsArg(0), path, sizeof(path));
	xsResult = xsAwaitImport(path, XS_IMPORT_DEFAULT);
}

void xs_modules_has(xsMachine *the)
{
	char name[C_PATH_MAX];

	xsToStringBuffer(xsArg(0), name, sizeof(name));
	xsResult = xsAwaitImport(name, XS_IMPORT_DEFAULT | XS_IMPORT_PREFLIGHT);
}
