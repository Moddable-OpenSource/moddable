#include "xsAll.h"
#include "mc.xs.h"
#include "xs.h"
#include "xsAll.h"
#include "xsHost.h"
#include <fcntl.h>
#include <unistd.h>
#include <execinfo.h> // Add header file for retrieving call stack
#include <signal.h>   // Add header file for signal handling
#include <pthread.h>  // Add pthread header file

#define ITERATION_TIMEOUT_US 5000000 // Set 5 second timeout threshold
#define FATAL_ERROR_TOKEN "\n\n!!!FATAL ERROR!!!\n\n"
extern txPreparation *xsPreparation;

// Use thread-local storage to save txMachine pointer
static __thread txMachine *gxMachine = NULL;

#if mxInstrument
gboolean on_instrumentation_timeout(gpointer data)
{
  // Execute your code here
  // Return TRUE if you want to continue calling, or FALSE if you want to stop after executing once
  txMachine *the = (void *)data;
  fxSampleInstrumentation(the, 0, NULL);
  // Reset
  the->garbageCollectionCount = 0;
	the->stackPeak = the->stack;
	the->peakParserSize = 0;
	the->floatingPointOps = 0;
	the->promisesSettledCount = 0;

  return TRUE; // Continue calling
}
#endif

// Modify dump_js_stack function, add thread check
void dump_js_stack(txMachine* the) {
  fprintf(stderr, "JavaScript stack trace:\n");
  txSlot *aFrame = the->frame;
	while (aFrame) {
		char name[128] = "";
		fxBufferFrameName(the, name, sizeof(name), aFrame, "");

		txSlot* environment = mxFrameToEnvironment(aFrame);
		if (environment->ID != XS_NO_ID)
			printf("%s: %s:%d\n", name, fxGetKeyName(the, environment->ID), environment->value.environment.line);
		else
			printf("%s\n", name);

		aFrame = aFrame->next;
	}
}

static void fatal_error_exit() {
  void *buffer[50];
  int nptrs = backtrace(buffer, 50);
  char **strings = backtrace_symbols(buffer, nptrs);
  printf("==== Native stack trace: =====\n");
  for (int i = 0; i < nptrs; i++)
  {
    printf("%s\n", strings[i]);
  }
  free(strings);
  printf("==============================\n");
  printf(FATAL_ERROR_TOKEN);
  
  signal(SIGQUIT, SIG_DFL);
  exit(1);
}

static void fatal_error_handler(int signum) {
  const char* signame = "UNKNOWN";
  switch(signum) {
    case SIGABRT: signame = "SIGABRT"; break;
    case SIGFPE:  signame = "SIGFPE"; break;
    case SIGILL:  signame = "SIGILL"; break;
    case SIGINT:  signame = "SIGINT"; break;
    case SIGQUIT: signame = "SIGQUIT"; break;
    case SIGSEGV: signame = "SIGSEGV"; break;
    case SIGTERM: signame = "SIGTERM"; break;
    case SIGBUS:  signame = "SIGBUS"; break;
    case SIGPIPE: signame = "SIGPIPE"; break;
  }
  
  printf("!!!! Signal %s (%d) caught. Stack trace:\n", signame, signum);
  // Reset signal handler to allow default behavior to terminate program
  signal(signum, SIG_DFL);
  fatal_error_exit();  
}

// Modified: Unified signal handling function
static void signal_handler(int signum) {
  fatal_error_handler(signum);
}

static void timeout_handler(int signum) {
  if (gxMachine) {
    printf("!!!! Execution timeout !!!!\n");
    dump_js_stack(gxMachine);
    fatal_error_exit();
  }
}

int main(int argc, char *argv[]) {
  // Use setvbuf to disable buffering
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  // Register all signals to be captured
  signal(SIGABRT, signal_handler);
  signal(SIGFPE, signal_handler);
  signal(SIGILL, signal_handler);
  signal(SIGQUIT, signal_handler);
  signal(SIGSEGV, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGBUS, signal_handler);
  signal(SIGPIPE, signal_handler);
  signal(SIGALRM, timeout_handler); // Used for timeout

  int error = 0;
  static txMachine _root;
  txMachine *the = &_root;
  txPreparation *preparation = xsPreparation();

  c_memset(the, 0, sizeof(txMachine));
  the->preparation = preparation;
  the->keyArray = preparation->keys;
  the->keyCount = (txID)preparation->keyCount + (txID)preparation->creation.initialKeyCount;
  the->keyIndex = (txID)preparation->keyCount;
  the->nameModulo = preparation->nameModulo;
  the->nameTable = preparation->names;
  the->symbolModulo = preparation->symbolModulo;
  the->symbolTable = preparation->symbols;

  the->stack = &preparation->stack[0];
  the->stackBottom = &preparation->stack[0];
  the->stackTop = &preparation->stack[preparation->stackCount];

  the->firstHeap = &preparation->heap[0];
  the->freeHeap = &preparation->heap[preparation->heapCount - 1];
  the->aliasCount = (txID)preparation->aliasCount;

  setvbuf(stdout, NULL, _IONBF, 0);
  the = fxCloneMachine(&preparation->creation, the, "linemb", NULL);
  gxMachine = the;  // Save to thread-local storage
#if mxInstrument
  fxDescribeInstrumentation(the, 0, NULL, NULL);
#endif
  xsBeginHost(the);
  {
    xsVars(2);
    {
      // XS: set global string array argv, and put input string into it
      xsTry
      {
        xsResult = xsNewArray(0);
        xsSet(xsGlobal, xsID("argv"), xsResult);
        for (int i = 0; i < argc; i++)
        {
          xsVar(0) = xsString(argv[i]);
          xsCall1(xsResult, xsID("push"), xsVar(0));
        }
      }
      xsCatch
      {
        xsStringValue message = xsToString(xsException);
        fprintf(stderr, "### %s\n", message);
        error = 1;
      }
    }
    {
      xsResult = xsAwaitImport("main", XS_IMPORT_NAMESPACE);
    }
  }
  xsEndHost(the);

  // Start event loop
  GMainContext *main_context = g_main_context_default();
  GMainLoop *main_loop = g_main_loop_new(main_context, FALSE);

#if mxInstrument
  g_timeout_add_seconds(1, on_instrumentation_timeout, (void *)the);
#endif

  // g_main_loop_run(main_loop);
  int testTime = 0;
  while (TRUE)
  {
    // Set timer: trigger SIGALRM on timeout
    struct itimerval timer;
    timer.it_value.tv_sec = ITERATION_TIMEOUT_US / 1000000;
    timer.it_value.tv_usec = ITERATION_TIMEOUT_US % 1000000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    // Record time before event handling
    guint64 start = g_get_monotonic_time();
    // Process one event non-blocking
    gboolean processed = g_main_context_iteration(NULL, FALSE);
    // Record time after event handling
    guint64 end = g_get_monotonic_time();

    // Disable timer
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    if (processed)
    {
      // printf("process time: %" G_GUINT64_FORMAT " us\n", end - start);
    }
  }

  xsDeleteMachine(the);

  return error;
}

const char *gXSAbortStrings[] ICACHE_FLASH_ATTR = {
	"debugger",
	"memory full",
	"stack overflow",
	"fatal",
	"dead strip",
	"unhandled exception",
	"not enough keys",
	"too much computation",
	"unhandled rejection"
};

void fxAbort(xsMachine *the, int status)
{
  const char *msg = (status <= XS_UNHANDLED_REJECTION_EXIT) ? gXSAbortStrings[status] : "unknown";
#if 0 // MODDEF_XS_ABORTHOOK

  if ((XS_STACK_OVERFLOW_EXIT != status) && (XS_DEBUGGER_EXIT != status)) {
    xsBooleanValue ignore = false;
    
    fxBeginHost(the);
    {
      mxPush(mxException);
      txSlot *exception = the->stack;
      mxException = xsUndefined;
      mxTry(the) {
        txID abortID = fxFindName(the, "abort");
        mxOverflow(-8);
        mxPush(mxGlobal);
        if (fxHasID(the, abortID)) {
          mxPush(mxGlobal);
          fxCallID(the, abortID);
          mxPushStringC((char *)msg);
          mxPushSlot(exception);
          fxRunCount(the, 2);
          ignore = (XS_BOOLEAN_KIND == the->stack->kind) && !the->stack->value.boolean;
          mxPop();
        }
      }
      mxCatch(the) {
      }
    }
    fxEndHost(the);
    if (ignore)
      return;
  }
#endif
  printf("Aborting with status %d, %s\n", status, msg);

  fatal_error_exit();
}