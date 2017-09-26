puts "---- Package search paths ----"
foreach p $auto_path {
    puts "$p"
}

if {$argc != 1} {
    error "Must specify 'tclpython or tclpython3'"
}

#===============================================================================
puts "---- Loading package [lindex $argv 0] ----"
switch [lindex $argv 0] {
    tclpython {
        package require tclpython
        set interp [python::interp new]
        set python python
    }
    
    tclpython3 {
        package require tclpython3
        set interp [python3::interp new]
        if {$::tcl_platform(platform) == "windows"} {
            # Windows does not distinguish between Python versions
            set python python
        } else {
            set python python3
        }
    }
    
    default {
        error "Invalid package '[lindex $argv 0] '"
    }
}
puts "OK"

#===============================================================================
puts "---- Verifying python version ----"
$interp exec {import sys}
set expected [exec $python -c {import sys;print('%d.%d.%d'%sys.version_info[0:3])}]
set actual [$interp eval {"%d.%d.%d" % sys.version_info[0:3]}]
if {$expected != $actual} {error "version check"}
puts "OK"

#===============================================================================
puts "---- Testing exception handler ----"
catch {
    $interp exec {raise Exception}
}
puts "OK"
