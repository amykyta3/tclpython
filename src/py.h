
#ifndef PY_H
#define PY_H

typedef struct py_interp_s py_interp_t;

py_interp_t* python_new_interpreter(void);
void python_delete_interpreter(py_interp_t *interp);

/**
* \brief Call Python interpreter via exec() function
* \returns 0 if OK. Nonzero if an exception occurred
**/
int python_exec(py_interp_t *interp, const char *str);

/**
* \brief Call Python interpreter via eval() function
* \returns Pointer to result string. NULL if an exception occurred.
*          Pointer should be deallocated with free() after done using
**/
char* python_eval(py_interp_t *interp, const char *str);

#endif
