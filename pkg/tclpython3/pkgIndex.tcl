switch -- $::tcl_platform(platform) {
  unix    {
    package ifneeded tclpython3 4.1 "load \"[file join $dir tclpython3.so.4.1]\""
  }
  windows {
    package ifneeded tclpython3 4.1 "load \"[file join $dir tclpython3-4.1.dll]\""
  }
  default {error "Unsupported platform: \"$::tcl_platform(platform)\""}
}
