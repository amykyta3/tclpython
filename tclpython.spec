# retain only major and minor version: "Python 2.2.3" gives 2.2
%define	pythonversion %(python -V 2>&1 | cut -d ' ' -f 2 | cut -d . -f 1,2)

Summary: Embedded Python interpreters package for Tcl
Name: tclpython
Version: 4.1
Release: 1%{?dist}
License: LGPL
Group: Development/Languages
Source: http://jfontain.free.fr/tclpython-4.1.tar.bz2
URL: http://jfontain.free.fr/
Packager: Jean-Luc Fontaine <jfontain@free.fr>
Requires: tcl >= 8.3.3, python >= %pythonversion
BuildRequires: %_includedir/tcl.h, python-devel >= %pythonversion
BuildRoot: %_tmppath/%name-%version-%release-root-%(%__id_u -n)

%description
Allows the evaluation or execution of Python code from a Tcl
interpreter, by creating one or several embedded Python interpreters
from the Tcl interpreter.

%prep

%setup -q

%build
cc -shared $RPM_OPT_FLAGS -s -o %name.so.%version -fPIC -DUSE_TCL_STUBS -I%_includedir/python%pythonversion %name.c tclthread.c %_libdir/libtclstub`echo 'puts $tcl_version' | %_bindir/tclsh`.a -L%_libdir/python%pythonversion/config -lpython%pythonversion -lpthread -lutil

%install
DIRECTORY=$RPM_BUILD_ROOT%_libdir/%name-%version
mkdir -p $DIRECTORY
echo 'package ifneeded %name %version "load [file join $dir %name.so.%version]"' >$DIRECTORY/pkgIndex.tcl
cp -p %name.so.%version $DIRECTORY

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc README CHANGES %name.htm
%_libdir/%name-%version


%changelog

* Sun Mar 5 2006 Jean-Luc Fontaine <jfontain@free.fr> 4.1-1
- 4.1 upstream release
- changed license to LGPL
- added distribution to release
- removed epochs

* Sat Apr 23 2005 Jean-Luc Fontaine <jfontain@free.fr> 4.0-1
- python side can now invoke code in parent Tcl interpreter

* Fri Jul 9 2004 Jean-Luc Fontaine <jfontain@free.fr> 0:3.1-0.fdr.2
- rebuilt for python 2.3 on Fedora Core 2

* Tue Dec 24 2003 Jean-Luc Fontaine <jfontain@free.fr> 0:3.1-0.fdr.1
- first Fedora Core 1 compatible package
