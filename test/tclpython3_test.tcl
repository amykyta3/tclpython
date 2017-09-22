
package require tclpython3
set python [python3::interp new]

$python exec {import sys}
puts [$python eval {"Python %d.%d.%d" % sys.version_info[0:3]}]
