#include "lin_xs.h"

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

extern void fxRunPromiseJobs(void* machine);
extern txS1 fxPromiseIsPending(xsMachine* the, xsSlot* promise);

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
				xsVar(2) = xsCallFunction1(xsVar(1), xsUndefined, xsVar(0));
				if (!xsIsInstanceOf(xsVar(2), xsPromisePrototype)) {
					fprintf(stderr, "main() returned immediate value (not a promise). exiting\n");
					exit(xsToInteger(xsVar(2)));
				}
				printf(" lin_xs_cli: main() returned a promise; entering event loop\n");

				GMainContext *main = g_main_context_default();
				while (fxPromiseIsPending(the, &xsVar(2))) {
					while (the->promiseJobsFlag) {
						the->promiseJobsFlag = 0;
						fxRunPromiseJobs(the);
					}
					g_main_context_iteration(main, TRUE);
				}
				g_main_context_unref(main);
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
