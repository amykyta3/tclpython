
if {$argc != 1} {
    error "Must specify 'tclpython or tclpython3'"
}

switch [lindex $argv 0] {
    tclpython {
        package require tclpython
        set interp [python::interp new]
        set python python
    }
    
    tclpython3 {
        package require tclpython3
        set interp [python3::interp new]
        set python python3
    }
    
    default {
        error "Invalid package '[lindex $argv 0] '"
    }
}


proc assert condition {
    if {![uplevel 1 expr $condition]} {
        return -code error "assertion failed: $condition"
    }
}

# Verify python version
$interp exec {import sys}
set expected [exec $python -c {import sys;print('%d.%d.%d'%sys.version_info[0:3])}]
set actual [$interp eval {"%d.%d.%d" % sys.version_info[0:3]}]
if {$expected != $actual} {error "version check"}

# Test exception handler
catch {
    $interp exec {raise Exception}
}
