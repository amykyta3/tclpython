[![build](https://github.com/amykyta3/tclpython/workflows/build/badge.svg)](https://github.com/amykyta3/tclpython/actions?query=workflow%3Abuild+branch%3Amaster)

# TclPython

This package allows the execution of Python code from a Tcl interpreter.

The Python interpreter is accessed using the `tclpython3` package:

```tcl
package require tclpython3
set interpreter [python3::interp new]
$interpreter exec {print("Hello World")}
puts [$interpreter eval 3/2.0]
python3::interp delete $interpreter
```


## Installing (Linux)

### 1. Install Dependencies

#### Debian
```bash
sudo apt install python3-dev tcl-dev
```

#### Red Hat
```bash
sudo yum install python3-devel tcl-devel
```

### 2. Determine install path
Tcl package installation paths vary depending on the platform.

Either pick one that looks right from the output of the following:

```bash
echo 'foreach p $auto_path {puts $p}' | tclsh
```

... or create a new location, and add it to the `TCLLIBPATH` environment variable:

```bash
export TCLLIBPATH=$TCLLIBPATH:/path/to/my/tcl/packages
```

### 3. Compile from source

```bash
make
make test
make install INSTALL_DIR=path/from/step/2
```

## Installing (Windows)
See [msvc/README.md](msvc/README.md)
