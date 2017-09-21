/* copyright (C) 2001 Jean-Luc Fontaine (mailto:jfontain@free.fr) */
/* this library is free software: please read the README file enclosed in this package */

/* $Id: tclpython.c,v 1.25 2006/03/05 17:41:53 jfontain Exp $ */

/*

Provide Python interpreters accessible from Tcl as a package named "tclpython".

Created using the following command:

# Linux Red Hat:
$ cc -shared -o tclpython.so.4.1 -s -fPIC -O2 -Wall -I/usr/include/python2.3 tclpython.c tclthread.c -L/usr/lib/python2.3/config -lpython2.3 -lpthread -lutil
# with Tcl stubs enabled:
$ cc -shared -o tclpython.so.4.1 -s -fPIC -O2 -Wall -DUSE_TCL_STUBS -I/usr/include/python2.3 tclpython.c tclthread.c /usr/lib/libtclstub8.3.a -L/usr/lib/python2.3/config -lpython2.3 -lpthread -lutil

# BSD (example, thanks to Dave Bodenstab):
$ cc -fpic -I/usr/local/include/python -I/usr/local/include/tcltk/tcl8.3 -c tclpython.c
$ cc -fpic -I/usr/local/include/tcltk/tcl8.3 -c tclthread.c
$ ld -o tclpython.so -Bshareable -L/usr/X11R6/lib -L/usr/local/lib -L/usr/local/share/python/config tclpython.o tclthread.o -lpython -lutil -lreadline -ltermcap -lcrypt -lgmp -lgdbm -lpq -lz -ltcl83 -ltk83 -lX11

Patched for Python 3 with respect to https://github.com/facebook/fbthrift/blob/master/thrift/lib/py/protocol/fastbinary.c

*/

#include <Python.h>
#include <tcl.h>

#if PY_MAJOR_VERSION >= 3
    #define PyInt_FromLong PyLong_FromLong
    #define PyInt_AsLong PyLong_AsLong
    #define PyString_FromStringAndSize PyBytes_FromStringAndSize
#else
    #include <cStringIO.h>
#endif

#include "tclpython.h"

// Mostly copied from cStringIO.c
#if PY_MAJOR_VERSION >= 3

/** io module in python3. */
static PyObject* Python3IO;

typedef struct {
  PyObject_HEAD
  char *buf;
  Py_ssize_t pos, string_size;
} IOobject;

#define IOOOBJECT(O) ((IOobject*)(O))

static int
IO__opencheck(IOobject *self) {
    if (!self->buf) {
        PyErr_SetString(PyExc_ValueError,
                        "I/O operation on closed file");
        return 0;
    }
    return 1;
}

static PyObject *
IO_cgetval(PyObject *self) {
    if (!IO__opencheck(IOOOBJECT(self))) return NULL;
    assert(IOOOBJECT(self)->pos >= 0);
    return PyBytes_FromStringAndSize(((IOobject*)self)->buf,
                                     ((IOobject*)self)->pos);
}
#endif

/* -- PYTHON MODULE SETUP STUFF --- */

static PyObject *pythonTclEvaluate(PyObject *self, PyObject *args);

static PyMethodDef tclMethods[] = {
    {"eval", pythonTclEvaluate, METH_VARARGS, "Evaluate a Tcl script."},
    {0, 0, 0, 0}                                                                                                      /* sentinel */
};

#if PY_MAJOR_VERSION >= 3
struct module_state {
  PyObject *error;
};

static struct PyModuleDef TclModuleDef = {
  PyModuleDef_HEAD_INIT,
  "tcl",
  NULL,
  sizeof(struct module_state),
  tclMethods,
  NULL,
  NULL,
  NULL,
  NULL
};
#endif

#ifndef WIN32
/* George Petasis, 21 Feb 2006:
 * The following check cannot be handled correctly
 * by Visual C++ preprocessor. */
#if (TCL_MAJOR_VERSION < 8)
    #error "Tcl 8.0 or greater is needed to build this"
#elif defined(USE_TCL_STUBS) && (TCL_MAJOR_VERSION == 8) &&\
    ((TCL_MINOR_VERSION == 0) || ((TCL_MINOR_VERSION == 1) && (TCL_RELEASE_LEVEL != TCL_FINAL_RELEASE)))
    #error "stubs interface does not work in 8.0 and alpha/beta 8.1, only 8.1.0+"
#endif
#endif

#ifdef _MSC_VER
    /* only do this when MSVC++ is used to compile */
    #ifdef USE_TCL_STUBS
        /* mark this .obj as needing Tcl stubs library */
        #pragma comment(lib, "tclstub" STRINGIFY(JOIN(TCL_MAJOR_VERSION,TCL_MINOR_VERSION)) ".lib")
        #if !defined(_MT) || !defined(_DLL) || defined(_DEBUG)
            /* fixes bug with how stubs library was compiled. requirement for msvcrt.lib from tclstubXX.lib should be removed. */
            #pragma comment(linker, "-nodefaultlib:msvcrt.lib")
        #endif
    #else
        /* mark this .obj as needing the import library */
        #pragma comment(lib, "tcl" STRINGIFY(JOIN(TCL_MAJOR_VERSION,TCL_MINOR_VERSION)) ".lib")
    #endif
#endif

static unsigned existingInterpreters = 0;
static struct Tcl_HashTable threadStates;
static struct Tcl_HashTable dictionaries;
static int newIdentifier;
#ifdef WITH_THREAD
static PyThreadState *globalState = 0;
#endif

static Tcl_Interp *mainInterpreter;                                                 /* needed for Tcl evaluation from Python side */
static Tcl_ThreadId mainThread;                                 /* needed for Python threads, 0 if Tcl core is not thread-enabled */

static int pythonInterpreter(ClientData clientData, Tcl_Interp *interpreter, int numberOfArguments, Tcl_Obj * CONST arguments[])
{
    intptr_t identifier;
    PyObject *output;
    PyObject *message;
    PyObject *result;
    PyObject *globals;
    char *string = 0;
    Py_ssize_t length;
    Tcl_Obj *object;
    struct Tcl_HashEntry *entry;
    unsigned evaluate;
#ifdef WITH_THREAD
    PyThreadState *state;
#endif

    if (numberOfArguments != 3) {
        Tcl_WrongNumArgs(interpreter, 1, arguments, "eval script");
        return TCL_ERROR;
    }
    string = Tcl_GetString(arguments[1]);
    evaluate = (strcmp(string, "eval") == 0);                                                       /* else the action is execute */
    if (!evaluate && (strcmp(string, "exec") != 0)) {
        object = Tcl_NewObj();
        Tcl_AppendStringsToObj(object, "bad option \"", string, "\": must be eval or exec", 0);
        Tcl_SetObjResult(interpreter, object);
        return TCL_ERROR;
    }
    identifier = atoi(Tcl_GetString(arguments[0]) + 6);                              /* interpreter and command name is "pythonN" */
    entry = Tcl_FindHashEntry(&threadStates, (ClientData)identifier);
    if (entry == 0) {
        object = Tcl_NewObj();
        Tcl_AppendStringsToObj(
            object, "invalid interpreter \"", Tcl_GetString(arguments[0]), "\": internal error, please report to tclpython author",
            0
        );
        Tcl_SetObjResult(interpreter, object);
        return TCL_ERROR;
    }
    globals = Tcl_GetHashValue(Tcl_FindHashEntry(&dictionaries, (ClientData)identifier));
#ifdef WITH_THREAD
    state = Tcl_GetHashValue(entry);
    PyEval_RestoreThread(state);                              /* acquire the global interpreter lock and make this thread current */
#endif
    /* choose start token depending on whether this is an evaluation or an execution: */
    result = PyRun_String(Tcl_GetString(arguments[2]), (evaluate? Py_eval_input: Py_file_input), globals, globals);
    if (result == 0) {                                                                                        /* an error occured */
#if PY_MAJOR_VERSION >= 3
        output = PyObject_CallMethod(Python3IO, "BytesIO", "()");
#else
        output = PycStringIO->NewOutput(1024);               /* use a reasonable initial size but big enough to handle most cases */
#endif
        PySys_SetObject("sys.stderr", output);                                                /* capture all interpreter error output */
        PyErr_Print();                                            /* so that error is printed on standard error, redirected above */
#if PY_MAJOR_VERSION >= 3
        message = IO_cgetval(output);
        string = PyBytes_AsString(message);
        length = (string == NULL) ? 0 : strlen(string);
#else
        message = PycStringIO->cgetvalue(output);
        string = PyString_AsString(message);
        length = PyString_Size(message);
#endif
        if ((length > 0) && (string[length - 1] == '\n')) length--;              /* eventually remove trailing new line character */
        object = Tcl_NewObj();
        Tcl_AppendStringsToObj(object, Tcl_GetString(arguments[0]), ": ", 0);                    /* identify interpreter in error */
        Tcl_AppendObjToObj(object, Tcl_NewStringObj(string, length));
        Py_DECREF(output);
    } else {
        if (evaluate) {
#if PY_MAJOR_VERSION >= 3
            string = PyUnicode_AsUTF8(PyObject_Str(result));
#else
            string = PyString_AsString(PyObject_Str(result));
#endif
            object = Tcl_NewStringObj(string, -1);                                                    /* return evaluation result */
        } else                                                                                                         /* execute */
            object = Tcl_NewObj();                                                   /* always return an empty result or an error */
        Py_DECREF(result);
    }
#ifdef WITH_THREAD
    PyEval_SaveThread();                        /* eventually restore the previous thread and release the global interpreter lock */
#endif
    Tcl_SetObjResult(interpreter, object);
    return(result == 0? TCL_ERROR: TCL_OK);
}

Tcl_Interp *tclInterpreter(CONST char *name)                           /* public function for use in extensions to this extension */
{
    intptr_t identifier;

    if ((sscanf(name, "tcl%lu", &identifier) == 0) || (identifier != 0)) {
        return 0;                                                                                                 /* invalid name */
    } else {
        return mainInterpreter;                                                                     /* sole available interpreter */
    }
}

static int tclEvaluate(CONST char *name, CONST char *script, char **string, int *length)              /* returns true if no error */
{
    Tcl_Interp *interpreter;
    int result;
    Tcl_Obj *object;

    interpreter = tclInterpreter(name);
    if (interpreter == 0) {
        object = Tcl_NewObj();
        Tcl_AppendStringsToObj(object, "invalid Tcl interpreter name: ", name, 0);
        *string = Tcl_GetStringFromObj(object, length);
        return 0;
    } else if (CURRENTTHREAD == mainThread) {                                                                /* non threaded code */
        Tcl_Preserve(interpreter);
        result = Tcl_EvalEx(interpreter, script, -1, TCL_EVAL_DIRECT | TCL_EVAL_GLOBAL);
        *string = Tcl_GetStringFromObj(Tcl_GetObjResult(interpreter), length);
        Tcl_Release(interpreter);
        return (result == TCL_OK);
    } else {                               /* threaded code: function like Tcl thread extension send command in asynchronous mode */
        tclSendThread(mainThread, interpreter, script);               /* let the interpreter in the main thread evaluate the code */
        *string = 0;                                                                                       /* nothing is returned */
        return 1;                                                       /* errors, if any, are reported on standard error channel */
    }
}

static PyObject *pythonTclEvaluate(PyObject *self, PyObject *args)
{
    CONST char *script;
    char *result;
    int length;

    if (!PyArg_ParseTuple(args, "s", &script))
        return 0;
    length = strlen(script);
    if (!tclEvaluate("tcl0", script, &result, &length)) {
        PyErr_SetString(PyExc_RuntimeError, result);
    }
    return Py_BuildValue("s", result);
}

static int newInterpreter(Tcl_Interp *interpreter)
{
    intptr_t identifier;
    Tcl_Obj *object;
    int created;
#ifdef WITH_THREAD
    PyThreadState *state;
#endif
    PyObject *tcl;

    identifier = newIdentifier;
#ifndef WITH_THREAD
    if (existingInterpreters > 0) {
        Tcl_SetResult(
            interpreter,
            "cannot create several concurrent Python interpreters\n(Python library was compiled without thread support)",
            TCL_STATIC
        );
        return TCL_ERROR;
    } else {
        Py_Initialize();                                                                           /* initialize main interpreter */
#if PY_MAJOR_VERSION >= 3 
        Python3IO = PyImport_ImportModule("io");
#else
        PycString_IMPORT;
#endif
    }
    Tcl_SetHashValue(Tcl_CreateHashEntry(&threadStates, (ClientData)identifier, &created), 0);
#else
    if (existingInterpreters == 0) {
        Py_Initialize();                                                                           /* initialize main interpreter */
        PyEval_InitThreads();                                               /* initialize and acquire the global interpreter lock */
#if PY_MAJOR_VERSION >= 3 
        Python3IO = PyImport_ImportModule("io");
#else
        PycString_IMPORT;
#endif
        globalState = PyThreadState_Swap(0);                                                            /* save the global thread */
    } else {
        PyEval_AcquireLock();                                           /* needed in order to be able to create a new interpreter */
    }
#if PY_MAJOR_VERSION >= 3
    if (Python3IO == 0) {                                              /* make sure string input/output is properly initialized */
#else
    if (PycStringIO == 0) {                                              /* make sure string input/output is properly initialized */
#endif
        Tcl_SetResult(interpreter, "fatal error: could not initialize Python string input/output module", TCL_STATIC);
        return TCL_ERROR;
    }
    state = Py_NewInterpreter();          /* hangs here if automatic 'import site' on a new thread is allowed (set Py_NoSiteFlag) */
    if (state == 0) {
        PyEval_ReleaseLock();
        Tcl_SetResult(interpreter, "could not create a new interpreter: please report to tclpython author", TCL_STATIC);
        return TCL_ERROR;
    }
    PyEval_ReleaseLock();                                                                  /* release the global interpreter lock */
    Tcl_SetHashValue(Tcl_CreateHashEntry(&threadStates, (ClientData)identifier, &created), state);
#endif
    Tcl_SetHashValue(
        Tcl_CreateHashEntry(&dictionaries, (ClientData)identifier, &created), PyModule_GetDict(PyImport_AddModule("__main__"))
    );
    object = Tcl_NewStringObj("python", -1);
    Tcl_AppendObjToObj(object, Tcl_NewIntObj(identifier));                          /* return "pythonN" as interpreter identifier */
    Tcl_SetObjResult(interpreter, object);
    Tcl_CreateObjCommand(interpreter, Tcl_GetString(object), pythonInterpreter, 0, 0);      /* create command for new interperter */
#ifdef WITH_THREAD
    newIdentifier++;
#endif
    existingInterpreters++;
#if PY_MAJOR_VERSION >= 3
    tcl = PyModule_Create(&TclModuleDef);
#else
    tcl = Py_InitModule("tcl", tclMethods);                                   /* add a new 'tcl' module to the python interpreter */
#endif
    Py_INCREF(tcl);
    PyModule_AddObject(PyImport_AddModule("__builtin__"), "tcl", tcl);
    return TCL_OK;
}

static int deleteInterpreters(Tcl_Interp *interpreter, int numberOfArguments, Tcl_Obj * CONST arguments[])
{
    int index;
    char *name;
    intptr_t identifier;
    struct Tcl_HashEntry *entry;
    Tcl_Obj *object;
#ifdef WITH_THREAD
    PyThreadState *state;
#endif

    for (index = 0; index < numberOfArguments; index++) {
        name = Tcl_GetString(arguments[index]);                                                  /* interpreter name is "pythonN" */
        entry = 0;
        if (sscanf(name, "python%lu", &identifier) == 1) {
            identifier = atoi(name + 6);
            entry = Tcl_FindHashEntry(&threadStates, (ClientData)identifier);
        }
        if (entry == 0) {
            object = Tcl_NewObj();
            Tcl_AppendStringsToObj(object, "invalid interpreter \"", name, 0);
            Tcl_SetObjResult(interpreter, object);
            return TCL_ERROR;
        }
#ifdef WITH_THREAD
        state = Tcl_GetHashValue(entry);
        PyEval_AcquireThread(state);                          /* acquire the global interpreter lock and make this thread current */
        Py_EndInterpreter(state);
        PyEval_ReleaseLock();
#endif
        Tcl_DeleteHashEntry(entry);
        Tcl_DeleteHashEntry(Tcl_FindHashEntry(&dictionaries, (ClientData)identifier));
        existingInterpreters--;
        if (existingInterpreters == 0) {                                                         /* no remaining sub-interpreters */
#ifdef WITH_THREAD
            PyEval_AcquireThread(globalState);                                                    /* required before finalization */
            globalState = 0;
#endif
            Py_Finalize();                                                                                 /* clean everything up */
        }
    }
    return TCL_OK;
}

static int command(ClientData clientData, Tcl_Interp *interpreter, int numberOfArguments, Tcl_Obj * CONST arguments[])
{
    char *command;
    unsigned new;
    unsigned delete;
    Tcl_Obj *object;

    if (numberOfArguments < 2) {
        Tcl_WrongNumArgs(interpreter, 1, arguments, "new|delete ?interp interp ...?");
        return TCL_ERROR;
    }
    command = Tcl_GetString(arguments[1]);
    new = (strcmp(command, "new") == 0);
    delete = (strcmp(command, "delete") == 0);
    if (!new && !delete) {
        object = Tcl_NewObj();
        Tcl_AppendStringsToObj(object, "bad option \"", command, "\": must be new or delete", 0);
        Tcl_SetObjResult(interpreter, object);
        return TCL_ERROR;
    }
    if (new) {
        if (numberOfArguments != 2) {
            Tcl_WrongNumArgs(interpreter, 1, arguments, "new");
            return TCL_ERROR;
        } else {
            return newInterpreter(interpreter);
        }
    }
    if (delete) {
#ifdef WITH_THREAD
        if (numberOfArguments < 3) {
            Tcl_WrongNumArgs(interpreter, 1, arguments, "delete ?interp interp ...?");
#else
        if (numberOfArguments != 3) {                                                        /* there can be one interpreter only */
            Tcl_WrongNumArgs(interpreter, 1, arguments, "delete interp");
#endif
            return TCL_ERROR;
        } else {
            return deleteInterpreters(interpreter, numberOfArguments -2, arguments + 2);
        }
    }
    return TCL_ERROR;                                                                                            /* never reached */
}

#ifdef WIN32
/* George Petasis, 21 Feb 2006:
 * Under Visual C++, functions exported from DLLs must be declared
 * with __declspec(dllexport) */
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif

EXTERN int Tclpython_Init(Tcl_Interp *interpreter)
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interpreter, "8.1", 0) == 0) {
        return TCL_ERROR;
    }
#endif
    Tcl_InitHashTable(&threadStates, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&dictionaries, TCL_ONE_WORD_KEYS);
    mainInterpreter = interpreter;
    mainThread = CURRENTTHREAD;
    newIdentifier = 0;
    Tcl_CreateObjCommand(interpreter, "::python::interp", command, 0, 0);
    Py_NoSiteFlag = 1;                      /* suppress automatic 'import site' to prevent interpreter from hanging on new thread */
    return Tcl_PkgProvide(interpreter, "tclpython", "4.1");
}
