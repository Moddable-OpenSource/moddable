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

extern void fxRunPromiseJobs(void* machine);
extern txS1 fxPromiseIsPending(xsMachine* the, xsSlot* promise);
extern txS1 fxPromiseIsRejected(xsMachine* the, xsSlot* promise);

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

				fprintf(stderr, "lin_xs_cli: loading top-level main.js\n");
				xsVar(1) = xsAwaitImport("main", XS_IMPORT_DEFAULT);
				fprintf(stderr, " lin_xs_cli: loaded\n");

				fprintf(stderr, "lin_xs_cli: invoking main(argv)\n");
				xsVar(2) = xsCallFunction1(xsVar(1), xsUndefined, xsVar(0));
				if (!xsIsInstanceOf(xsVar(2), xsPromisePrototype)) {
					fprintf(stderr, "main() returned immediate value (not a promise). exiting\n");
					exit(xsToInteger(xsVar(2)));
				}
				fprintf(stderr, " lin_xs_cli: main() returned a promise; entering event loop\n");

				GMainContext *mainctx = g_main_context_default();
				while (the->promiseJobsFlag || fxPromiseIsPending(the, &xsVar(2))) {
					while (the->promiseJobsFlag) {
						the->promiseJobsFlag = 0;
						fxRunPromiseJobs(the);
					}
					g_main_context_iteration(mainctx, TRUE);
				}
				if (fxPromiseIsRejected(the, &xsVar(2))) {
					error = 1;
				}
				// ISSUE: g_main_context_unref(mainctx); causes xsDeleteMachine() below
				//        to hang in g_main_context_find_source_by_id() aquiring a lock.
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
	return error;
}
