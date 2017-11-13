puts "---- Package search paths ----"
foreach p $auto_path {
    puts "$p"
}

if {$argc != 1} {
    error "Must specify 'tclpython or tclpython3'"
}

#===============================================================================
set tclpython [lindex $argv 0]
switch $tclpython {
    tclpython {
        set python_exe python
        set cmd_interp python::interp
    }
    
    tclpython3 {
        if {$::tcl_platform(platform) == "windows"} {
            # Windows does not distinguish between Python versions
            set python_exe python
        } else {
            set python_exe python3
        }
        set cmd_interp python3::interp
    }
    
    default {
        error "Invalid package '[lindex $argv 0] '"
    }
}

puts "---- Loading package $tclpython ----"
package require $tclpython
set interp [$cmd_interp new]
puts "OK"

#===============================================================================
puts "---- Verifying python version ----"
$interp exec {import sys}
set expected [exec $python_exe -c {import sys;print('%d.%d.%d'%sys.version_info[0:3])}]
set actual [$interp eval {"%d.%d.%d" % sys.version_info[0:3]}]
if {$expected != $actual} {error "version check"}
puts "OK"

#===============================================================================
puts "---- Testing exception handler ----"
catch {
    $interp exec {raise Exception}
}
puts "OK"

#===============================================================================

$cmd_interp delete $interp
