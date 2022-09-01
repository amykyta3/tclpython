puts "---- Package search paths ----"
foreach p $auto_path {
    puts "$p"
}

#===============================================================================
if {$::tcl_platform(platform) == "windows"} {
    # Windows does not distinguish between Python versions
    set python_exe python
} else {
    set python_exe python3
}

puts "---- Loading package tclpython3 ----"
package require tclpython3
set interp [python3::interp new]
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

python3::interp delete $interp
