switch -- $::tcl_platform(platform) {
  unix    {
    package ifneeded tclpython 4.1 "load [file join $dir tclpython.so.4.1]"
  }
  windows {
    package ifneeded tclpython 4.1 "::python::load_windows [file normalize $dir]"
  }
  default {error "Unsupported platform: \"$::tcl_platform(platform)\""}
}

namespace eval ::python {
  ## Try to locate all available DLLs, and try to load all of them, until
  ## a DLL gets loaded.
  proc load_windows {dir} {
    foreach dll [glob -directory $dir tclpython-4.1-*.dll] {
      if {![catch {load $dll} error]} {
        break
      }
    }
  };# load_windows
};# namespace ::python
