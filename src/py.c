#include <stdlib.h>
#include <string.h>

#include <Python.h>

#include "py.h"

struct py_interp_s {
    PyThreadState *thread_state;
    PyObject *globals;
};

static unsigned InterpCount = 0;
#ifdef WITH_THREAD
    static PyThreadState *GlobalThread = NULL;
#endif

//------------------------------------------------------------------------------
py_interp_t* python_new_interpreter(void){
    #ifdef WITH_THREAD
        PyThreadState *thread_state;

        if(InterpCount == 0){
            // First time initializing Python
            Py_NoSiteFlag = 1;
            Py_Initialize();
            #if (PY_MAJOR_VERSION == 3) && (PY_MINOR_VERSION >= 9)
                // deprecated in python3.9
            #else
                PyEval_InitThreads();
            #endif
            GlobalThread = PyEval_SaveThread();
        }

        PyEval_RestoreThread(GlobalThread);
        thread_state = Py_NewInterpreter();

        py_interp_t *interp;
        interp = malloc(sizeof(py_interp_t));
        interp->thread_state = thread_state;
        interp->globals = PyModule_GetDict(PyImport_AddModule("__main__"));

        PyEval_SaveThread();
    #else
        // Python was compiled without thread support
        // only allowed one interpreter
        if(InterpCount != 0){
            return(NULL);
        }

        Py_Initialize();

        py_interp_t *interp;
        interp = malloc(sizeof(py_interp_t));
        interp->globals = PyModule_GetDict(PyImport_AddModule("__main__"));
    #endif

    InterpCount++;
    return(interp);
}

//------------------------------------------------------------------------------
void python_delete_interpreter(py_interp_t *interp){
    #ifdef WITH_THREAD
        PyEval_RestoreThread(interp->thread_state);
        Py_EndInterpreter(interp->thread_state);
        PyThreadState_Swap(GlobalThread);
        PyEval_ReleaseThread(GlobalThread);
    #endif

    free(interp);
    InterpCount--;

    if(InterpCount == 0){
        // no remaining sub-interpreters
        // Clean up Python's interpreter
        #ifdef WITH_THREAD
            PyEval_RestoreThread(GlobalThread);
        #endif
        Py_Finalize();
    }
}

//------------------------------------------------------------------------------
int python_exec(py_interp_t *interp, const char *str){
    int res;
    #ifdef WITH_THREAD
        PyEval_RestoreThread(interp->thread_state);
    #endif

    PyObject *py_result;
    py_result = PyRun_String(str, Py_file_input, interp->globals, interp->globals);
    if(!py_result) {
        // an error occurred
        PyErr_PrintEx(1);
        res = -1;
    } else {
        Py_DECREF(py_result);
        res = 0;
    }
    #ifdef WITH_THREAD
        PyEval_SaveThread();
    #endif
    return(res);
}

//------------------------------------------------------------------------------
char* python_eval(py_interp_t *interp, const char *str){
    #ifdef WITH_THREAD
        PyEval_RestoreThread(interp->thread_state);
    #endif

    PyObject *py_result;
    char *result_str;
    py_result = PyRun_String(str, Py_eval_input, interp->globals, interp->globals);
    if(!py_result) {
        // an error occurred
        PyErr_PrintEx(1);
        result_str = NULL;
    } else {
        PyObject *py_result_str;
        char *tmp_str;

        // Get string representation of result
        py_result_str = PyObject_Str(py_result);
        tmp_str = PyUnicode_AsUTF8(py_result_str);

        // Copy string
        result_str = malloc(sizeof(char)*(strlen(tmp_str)+1));
        strcpy(result_str, tmp_str);

        // Free python result objects
        Py_DECREF(py_result_str);
        Py_DECREF(py_result);
    }
    #ifdef WITH_THREAD
        PyEval_SaveThread();
    #endif

    return(result_str);
}
