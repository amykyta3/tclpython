# Installing (Windows)

## 1. Install Dependencies
All dependeincies must use their 32-bit (x86) installers

* [MS Visual C++ Build Tools](http://landinghub.visualstudio.com/visual-cpp-build-tools)
* [ActiveState Tcl](https://www.activestate.com/activetcl/downloads)
* [Python (2.x and/or 3.x)](https://www.python.org/downloads)
    * Be sure to customize the installation and check: "Add Python to environment variables"

## 2: Edit Paths
Make sure paths to all installations are correct in your environment.

In `start_msvc_shell.bat`, edit variables:

* vcbuildtools
    * Path to the `vcbuildtools.bat` script located in the MS Visual C++ Build Tools intstallation

In `Makefile`, edit variables:

* TCL_PATH
    * Path to root of Tcl installation
* PYTHON2_PATH
    * Path to root of Python 2.x.x installation
* PYTHON3_PATH
    * Path to root of Python 3.x.x installation
* INSTALL_DIR
    * Destination of the tclpython Tcl package
    * Either pick one that looks right from the output of the following Tcl command:
        `foreach p $auto_path {puts $p}`
    * ... or create a new location, and add it to the `TCLLIBPATH` environment variable.

## 3. Compile

Double-click `start_msvc_shell.bat` to open the compiler command shell.


### For Python 2:

```bash
nmake
nmake install
```

### For Python 3:

```bash
nmake PKG_NAME=tclpython3
nmake install PKG_NAME=tclpython3
```
