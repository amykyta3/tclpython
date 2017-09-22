
package require tclpython
set python [python::interp new]

$python exec {import sys}
puts [$python eval {"Python %d.%d.%d" % sys.version_info[0:3]}]
