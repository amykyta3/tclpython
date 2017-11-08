
#include <Python.h>
#include <tcl.h>
#include "tclpython.h"

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

static Tcl_Interp *mainInterpreter; /* needed for Tcl evaluation from Python side */
static Tcl_ThreadId mainThread; /* needed for Python threads, 0 if Tcl core is not thread-enabled */

static int pythonInterpreter(ClientData clientData, Tcl_Interp *interpreter, int numberOfArguments, Tcl_Obj * CONST arguments[])
{
    intptr_t identifier;
    PyObject *result;
    PyObject *globals;
    char *string = 0;
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
        PyErr_Print();
        object = Tcl_NewObj();
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

static int newInterpreter(Tcl_Interp *interpreter)
{
    intptr_t identifier;
    Tcl_Obj *object;
    int created;
#ifdef WITH_THREAD
    PyThreadState *state;
#endif

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
    }
    Tcl_SetHashValue(Tcl_CreateHashEntry(&threadStates, (ClientData)identifier, &created), 0);
#else
    if (existingInterpreters == 0) {
        Py_Initialize();                                                                           /* initialize main interpreter */
        PyEval_InitThreads();                                               /* initialize and acquire the global interpreter lock */
        globalState = PyThreadState_Swap(0);                                                            /* save the global thread */
    } else {
        PyEval_AcquireLock();                                           /* needed in order to be able to create a new interpreter */
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

static int command(ClientData clientData, Tcl_Interp *interpreter, int numberOfArguments, Tcl_Obj * CONST arguments[]) {
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
    return TCL_ERROR; /* never reached */
}

#ifdef WIN32
    // George Petasis, 21 Feb 2006:
    // Under Visual C++, functions exported from DLLs must be declared
    // with __declspec(dllexport)
    #undef TCL_STORAGE_CLASS
    #define TCL_STORAGE_CLASS DLLEXPORT
#endif

EXTERN int Tclpython_Init(Tcl_Interp *interpreter) {
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
    #if PY_MAJOR_VERSION >= 3
        Tcl_CreateObjCommand(interpreter, "::python3::interp", command, 0, 0);
        Py_NoSiteFlag = 1;// suppress automatic 'import site' to prevent interpreter from hanging on new thread 
        return Tcl_PkgProvide(interpreter, "tclpython3", "4.2");
    #else
        Tcl_CreateObjCommand(interpreter, "::python::interp", command, 0, 0);
        Py_NoSiteFlag = 1;// suppress automatic 'import site' to prevent interpreter from hanging on new thread 
        return Tcl_PkgProvide(interpreter, "tclpython", "4.2");
    #endif
}
