
#include "xsPlatform.h"
#include "xs.h"
#include "mc.xs.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h>
#define mxSeparator '/'

#ifdef mxDebug
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowMessage(the,(char *)__FILE__,__LINE__,XS_UNKNOWN_ERROR,"%s",strerror(errno)), 1)))
#else
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowMessage(the,NULL,0,XS_UNKNOWN_ERROR,"%s",strerror(errno)), 1)))
#endif

static char** then = NULL;

void fxAbort(xsMachine* the)
{
	exit(1);
}

int main(int argc, char* argv[])  // here
{
	int error = 0;

	xsMachine* machine = fxPrepareMachine(NULL, xsPreparation(), "tool", NULL, NULL);

	xsBeginHost(machine);
	{
		xsVars(3);
		{
			xsTry {
				int argi;
				xsVar(0) = xsNewArray(0);
				for (argi = 1; argi < argc; argi++) {
					xsSetAt(xsVar(0), xsInteger(argi - 1), xsString(argv[argi]));
				}

				printf("lin_xs_cli: loading top-level main.js\n");
				xsVar(1) = xsAwaitImport("main", XS_IMPORT_DEFAULT);
				printf(" lin_xs_cli: loaded\n");

				printf("lin_xs_cli: invoking main(argv)\n");
				xsCallFunction1(xsVar(1), xsUndefined, xsVar(0));
				printf(" lin_xs_cli: invoked\n");
			}
			xsCatch {
				xsStringValue message = xsToString(xsException);
				fprintf(stderr, "### %s\n", message);
				error = 1;
			}
		}
	}
	xsEndHost(the);
	xsDeleteMachine(machine);
	if (!error && then) {
	#if mxWindows
		error =_spawnvp(_P_WAIT, then[0], then);
		if (error < 0)
			fprintf(stderr, "### Cannot execute %s!\n", then[0]);
	#else
		execvp(then[0], then);
	#endif
	}
	return error;
}
