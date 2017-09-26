# Installing (Windows)

## 1. Install Dependencies
All dependeincies must use their 32-bit (x86) installers

[MS Visual C++ Build Tools](http://landinghub.visualstudio.com/visual-cpp-build-tools)

[ActiveState Tcl](https://www.activestate.com/activetcl/downloads)

[Python (2.x and/or 3.x)](https://www.python.org/downloads)

## 2: Edit Paths
Make sure paths to all installations are correct in your environment.

In `start_msvc_shell.bat`, edit variables:

* `vcbuildtools`

In `Makefile`, edit variables:

* `TCL_PATH`
* `PYTHON2_PATH`
* `PYTHON3_PATH`
* `INSTALL_DIR`

## 3. Compile

Double-click `start_msvc_shell.bat` to open the compiler command shell.


### For Python 2:

```bash
nmake
nmake install
```

### For Python 3:

```bash
nmake TCL_PKG=tclpython3
nmake install TCL_PKG=tclpython3
```
