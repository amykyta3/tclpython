
/* when Tcl core was not compiled for multithreading, Tcl_GetCurrentThread() always returns 0, so */
/* use this macro instead to be able to detect whether request is coming from a different thread. */
#ifdef __WIN32__
    #include <Windows.h>
    #define CURRENTTHREAD ((Tcl_ThreadId)GetCurrentThreadId())
#else
    #include <pthread.h>
    #define CURRENTTHREAD ((Tcl_ThreadId)pthread_self())
#endif
extern void tclSendThread(Tcl_ThreadId, Tcl_Interp *, CONST char *);
/* public function for use in extensions to this extension: */
extern Tcl_Interp *tclInterpreter(CONST char *);
