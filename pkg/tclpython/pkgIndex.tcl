switch -- $::tcl_platform(platform) {
  unix    {
    package ifneeded tclpython 4.1 "load \"[file join $dir tclpython.so.4.1]\""
  }
  windows {
    package ifneeded tclpython 4.1 "load \"[file join $dir tclpython-4.1.dll]\""
  }
  default {error "Unsupported platform: \"$::tcl_platform(platform)\""}
}
