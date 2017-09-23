# TclPython

This package allows the execution of Python code from a Tcl interpreter.


The Python 2 interpreter is accessed using the `tclpython` package:

```tcl
package require tclpython
set interpreter [python::interp new]
$interpreter exec {print "Hello World"}
puts [$interpreter eval 3/2.0]
python::interp delete $interpreter
```

The Python 3 interpreter is accessed using the `tclpython3` package:

```tcl
package require tclpython3
set interpreter [python3::interp new]
$interpreter exec {print("Hello World")}
puts [$interpreter eval 3/2.0]
python::interp delete $interpreter
```

# Installing

## 1. Install Dependencies
```bash
sudo apt-get update
sudo apt-get install python-dev python3-dev tcl-dev
```

## 2. Determine install path
Tcl package installation paths vary depending on the platform.
Pick one that looks right from the output of the following:

```bash
echo 'foreach p $auto_path {puts $p}' | tclsh
```

## 3. Compile from source
Installations of the package for Python 2 and 3 can coexist.

**For Python 2:**

```bash
make
make install INSTALL_DIR=path/from/step/2
```
**For Python 3:**

```bash
make TCL_PKG=tclpython3
make install TCL_PKG=tclpython3 INSTALL_DIR=path/from/step/2
```

---
*Jean-Luc Fontaine  mailto:jfontain@free.fr  http://jfontain.free.fr/*
