
#include <string.h>
#include <tcl.h>
#include "tclpython.h"


TCL_DECLARE_MUTEX(threadMutex);

typedef struct ThreadEvent {                        /* copied from threadCmd.c in Tcl thread extension source code and simplified */
    Tcl_Event event;
    Tcl_Interp *interpreter;
    char *script;
} ThreadEvent;


static void ThreadErrorProc(Tcl_Interp *interpreter)
{
    char buffer[sizeof(Tcl_ThreadId)*2+1];
    CONST char *errorInformation;
    Tcl_Channel errorChannel;

    errorInformation = Tcl_GetVar(interpreter, "errorInfo", TCL_GLOBAL_ONLY);
    if (errorInformation == 0) {
        errorInformation = "";
    }
    errorChannel = Tcl_GetStdChannel(TCL_STDERR);
    if (errorChannel == NULL) return;
    sprintf(buffer, "%lX", (long)CURRENTTHREAD);
    Tcl_WriteChars(errorChannel, "Error from thread 0x", -1);
    Tcl_WriteChars(errorChannel, buffer, -1);
    Tcl_WriteChars(errorChannel, "\n", 1);
    Tcl_WriteChars(errorChannel, errorInformation, -1);
    Tcl_WriteChars(errorChannel, "\n", 1);
}

static int ThreadEventProc(Tcl_Event *event, int mask)
{
    int code;
    ThreadEvent *data = (ThreadEvent *)event;                                                    /* event is really a ThreadEvent */

    Tcl_Preserve(data->interpreter);
    code = Tcl_EvalEx(data->interpreter, data->script, -1, TCL_EVAL_GLOBAL);
    Tcl_Free(data->script);
    if (code != TCL_OK) {
        ThreadErrorProc(data->interpreter);
    }
    Tcl_Release(data->interpreter);
    return 1;
}

void tclSendThread(Tcl_ThreadId thread, Tcl_Interp *interpreter, CONST char *script)
{
    ThreadEvent *event;
    Tcl_Channel errorChannel;
    Tcl_Obj *object;
    int boolean;

    object = Tcl_GetVar2Ex(interpreter, "::tcl_platform", "threaded", 0);
    if ((object == 0) || (Tcl_GetBooleanFromObj(interpreter, object, &boolean) != TCL_OK) || !boolean) {
        errorChannel = Tcl_GetStdChannel(TCL_STDERR);
        if (errorChannel == NULL) return;
        Tcl_WriteChars(
            errorChannel, "error: Python thread requested script evaluation on Tcl core not compiled for multithreading.\n", -1
        );
        return;
    }
    event = (ThreadEvent *)Tcl_Alloc(sizeof(ThreadEvent));
    event->event.proc = ThreadEventProc;
    event->interpreter = interpreter;
    event->script = strcpy(Tcl_Alloc(strlen(script) + 1), script);
    Tcl_MutexLock(&threadMutex);
    Tcl_ThreadQueueEvent(thread, (Tcl_Event *)event, TCL_QUEUE_TAIL);
    Tcl_ThreadAlert(thread);
    Tcl_MutexUnlock(&threadMutex);
}
