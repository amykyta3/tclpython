/* copyright (C) 2001-2004 Jean-Luc Fontaine (mailto:jfontain@free.fr) */
/* this library is free software: please read the README file enclosed in this package */

/* $Id: tclpython.h,v 1.2 2006/03/05 17:41:53 jfontain Exp $ */

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
