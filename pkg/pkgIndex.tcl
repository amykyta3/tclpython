switch -- $::tcl_platform(platform) {
    unix    {
        # Register all possible versions in this dir
        foreach sharedlib [glob -type f -directory "$dir" "tclpython*.so.*"] {
            if {[regexp {(tclpython|tclpython3)\.so\.(\d+\.\d+)} [file tail $sharedlib] m libname version]} {
                package ifneeded $libname $version "load \"$sharedlib\""
            }
        }
    }
    windows {
        # Register all possible versions in this dir
        foreach sharedlib [glob -type f -directory "$dir" "tclpython*-*.dll"] {
            if {[regexp {(tclpython|tclpython3)-(\d+\.\d+)\.dll} [file tail $sharedlib] m libname version]} {
                package ifneeded $libname $version "load \"$sharedlib\""
            }
        }
    }
    default {
        error "Unsupported platform: \"$::tcl_platform(platform)\""
    }
}
