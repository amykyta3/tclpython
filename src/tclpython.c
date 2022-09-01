#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <Python.h>
#include <tcl.h>

#include "py.h"


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


static struct Tcl_HashTable interp_info_table;
static int newIdentifier;

//------------------------------------------------------------------------------
static int pythonInterpreter(ClientData clientData, Tcl_Interp *interpreter, int objc, Tcl_Obj *const objv[]) {
    intptr_t identifier;
    Tcl_Obj *object;
    int result;

    struct Tcl_HashEntry *I_entry;

    if (objc != 3) {
        Tcl_WrongNumArgs(interpreter, 1, objv, "eval|exec script");
        return(TCL_ERROR);
    }

    // interpreter name is "pythonN"
    I_entry = NULL;
    if(sscanf(Tcl_GetString(objv[0]), "python%lu", &identifier) == 1) {
        I_entry = Tcl_FindHashEntry(&interp_info_table, (ClientData)identifier);
    }

    if(!I_entry) {
        object = Tcl_NewObj();
        Tcl_AppendStringsToObj(
            object, "invalid interpreter \"", Tcl_GetString(objv[0]), "\": internal error, please report to tclpython author",
            0
        );
        Tcl_SetObjResult(interpreter, object);
        return(TCL_ERROR);
    }
    py_interp_t *py_interp;
    py_interp = Tcl_GetHashValue(I_entry);

    // choose start token depending on whether this is an evaluation or an execution
    char *subcommand;
    subcommand = Tcl_GetString(objv[1]);
    if(!strcmp(subcommand, "exec")){
        if(python_exec(py_interp, Tcl_GetString(objv[2]))) {
            result = TCL_ERROR;
        } else {
            result = TCL_OK;
        }
        // always return an empty result or an error
        object = Tcl_NewObj();
    } else if(!strcmp(subcommand, "eval")){
        char *result_str;
        result_str = python_eval(py_interp, Tcl_GetString(objv[2]));
        if(result_str) {
            object = Tcl_NewStringObj(result_str, -1);
            free(result_str);
            result = TCL_OK;
        } else {
            /* an error occurred */
            object = Tcl_NewObj();
            result = TCL_ERROR;
        }
    } else {
        object = Tcl_NewObj();
        Tcl_AppendStringsToObj(object, "bad option \"", subcommand, "\": must be eval or exec", 0);
        result = TCL_ERROR;
    }

    Tcl_SetObjResult(interpreter, object);
    return(result);
}

//------------------------------------------------------------------------------
static int newInterpreter(Tcl_Interp *interpreter){
    intptr_t identifier;
    int created;

    py_interp_t *py_interp;
    py_interp = python_new_interpreter();
    if(!py_interp){
        Tcl_SetResult(
            interpreter,
            "cannot create several concurrent Python interpreters\n(Python library was compiled without thread support)",
            TCL_STATIC
        );
        return(TCL_ERROR);
    }

    // Save python interpreter info
    identifier = newIdentifier;
    Tcl_SetHashValue(Tcl_CreateHashEntry(&interp_info_table, (ClientData)identifier, &created), py_interp);

    // Return "pythonN" and register it as a new command keyword
    Tcl_Obj *object;
    object = Tcl_NewStringObj("python", -1);
    Tcl_AppendObjToObj(object, Tcl_NewIntObj(identifier));
    Tcl_SetObjResult(interpreter, object);
    Tcl_CreateObjCommand(interpreter, Tcl_GetString(object), pythonInterpreter, NULL, NULL);

    newIdentifier++;
    return(TCL_OK);
}

//------------------------------------------------------------------------------
static int deleteInterpreter(Tcl_Interp *interpreter, char *name) {
    intptr_t identifier;
    struct Tcl_HashEntry *I_entry;
    Tcl_Obj *object;

    // interpreter name is "pythonN"
    I_entry = NULL;
    if(sscanf(name, "python%lu", &identifier) == 1) {
        I_entry = Tcl_FindHashEntry(&interp_info_table, (ClientData)identifier);
    }

    if(!I_entry) {
        object = Tcl_NewObj();
        Tcl_AppendStringsToObj(object, "invalid interpreter \"", name, 0);
        Tcl_SetObjResult(interpreter, object);
        return(TCL_ERROR);
    }

    py_interp_t *py_interp;
    py_interp = Tcl_GetHashValue(I_entry);
    python_delete_interpreter(py_interp);
    Tcl_DeleteHashEntry(I_entry);

    return(TCL_OK);
}

//------------------------------------------------------------------------------
/**
* \brief Implements the python::interp Tcl command
* Creates or deletes a Python interpreter instance
**/
static int cmd_interp(ClientData clientData, Tcl_Interp *interpreter, int objc, Tcl_Obj *const objv[]) {
    char *subcommand;
    Tcl_Obj *object;

    if(objc < 2){
        Tcl_WrongNumArgs(interpreter, 1, objv, "new|delete ?interp interp ...?");
        return(TCL_ERROR);
    }
    subcommand = Tcl_GetString(objv[1]);

    if(!strcmp(subcommand, "new")){
        if(objc != 2) {
            Tcl_WrongNumArgs(interpreter, 1, objv, "new");
            return(TCL_ERROR);
        } else {
            return(newInterpreter(interpreter));
        }
    } else if(!strcmp(subcommand, "delete")){
        if(objc < 3) {
            Tcl_WrongNumArgs(interpreter, 1, objv, "delete ?interp interp ...?");
            return(TCL_ERROR);
        } else {
            for(int i = 2; i<objc; i++){
                char *name;
                name = Tcl_GetString(objv[i]);
                if(deleteInterpreter(interpreter, name) != TCL_OK) return(TCL_ERROR);
            }
            return(TCL_OK);
        }

    } else {
        object = Tcl_NewObj();
        Tcl_AppendStringsToObj(object, "bad option \"", subcommand, "\": must be new or delete", 0);
        Tcl_SetObjResult(interpreter, object);
        return(TCL_ERROR);
    }

    // Should never reach
    return(TCL_ERROR);
}

//------------------------------------------------------------------------------
#ifdef WIN32
    // George Petasis, 21 Feb 2006:
    // Under Visual C++, functions exported from DLLs must be declared
    // with __declspec(dllexport)
    #undef TCL_STORAGE_CLASS
    #define TCL_STORAGE_CLASS DLLEXPORT
#endif

#define STR(s) #s
#define XSTR(s) STR(s)

/**
* \brief Initialize tclpython at package import
**/
EXTERN int Tclpython_Init(Tcl_Interp *interpreter){
    #ifdef USE_TCL_STUBS
    if(Tcl_InitStubs(interpreter, "8.1", 0) == 0) {
        return(TCL_ERROR);
    }
    #endif

    Tcl_InitHashTable(&interp_info_table, TCL_ONE_WORD_KEYS);
    newIdentifier = 0;

    Tcl_CreateObjCommand(interpreter, "::python3::interp", cmd_interp, NULL, NULL);
    return(Tcl_PkgProvide(interpreter, "tclpython3", XSTR(TCLPYTHON_VERSION)));
}
