 
# Version History
All versions are tagged with the corresponding version number.

## `5.0`
* Significant rewrite ([Issue #4](https://github.com/amykyta3/tclpython/issues/4))
    * Fix several python thread stability issues
    * Fix memory leak in python eval return value handling
* Remove builtin `tcl.eval()` function from Python interpreter environment
    * Use `tkinter.Tcl().eval()` instead.
    * See ([Issue #3](https://github.com/amykyta3/tclpython/issues/3))

## `4.2`
* Fix compatibility with latest compilers
* Add makefiles for Linux and Windows build systems
* Add support for Python3 via tclpython3

## `4.1`
* Fork of original TclPython from [http://jfontain.free.fr/](http://jfontain.free.fr/)
