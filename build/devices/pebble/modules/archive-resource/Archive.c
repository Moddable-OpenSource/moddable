#include "xsAll.h"
#include "xs.h"
#include "mc.xs.h"
#include "mc.defines.h"

void Archive_constructor(xsMachine *the)
{
	xsTypeError("new Archive");
}

void Archive_destructor(void* data)
{
}

void  Archive_get_modulePaths(xsMachine *the)
{
	void* archive = xsGetHostData(xsThis);
	xsIntegerValue c = fxGetArchiveCodeCount(the, archive), i;
	xsResult = xsNewArray(c);
	for (i = 0; i < c; i++) {
		xsStringValue name = fxGetArchiveCodeName(the, archive, i);
		xsSetIndex(xsResult, i, xsString(name));
	}
}

void Archive_get_name(xsMachine *the)
{
	void* archive = xsGetHostData(xsThis);
	xsStringValue name = fxGetArchiveName(the, archive);
	xsResult = xsString(name);;
}

void Archive_get_resourcePaths(xsMachine *the)
{
	void* archive = xsGetHostData(xsThis);
	xsIntegerValue c = fxGetArchiveDataCount(the, archive), i;
	xsResult = xsNewArray(c);
	for (i = 0; i < c; i++) {
		xsStringValue name = fxGetArchiveDataName(the, archive, i);
		xsSetIndex(xsResult, i, xsString(name));
	}
}

