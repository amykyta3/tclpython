PYTHON2_VERSION=2.7
PYTHON3_VERSION=3.5m
INSTALL_DIR=/usr/lib/tcltk/x86_64-linux-gnu


TCLPYTHON_ARGS=TCL_PKG_NAME=tclpython PYTHON_VERSION=$(PYTHON2_VERSION) INSTALL_DIR=$(INSTALL_DIR)
TCLPYTHON3_ARGS=TCL_PKG_NAME=tclpython3 PYTHON_VERSION=$(PYTHON3_VERSION) INSTALL_DIR=$(INSTALL_DIR)

all:
	$(MAKE) -f tclpython.mk $(TCLPYTHON_ARGS)
	$(MAKE) -f tclpython.mk $(TCLPYTHON3_ARGS)

test:
	$(MAKE) -f tclpython.mk $(TCLPYTHON_ARGS) test
	$(MAKE) -f tclpython.mk $(TCLPYTHON3_ARGS) test

install:
	$(MAKE) -f tclpython.mk $(TCLPYTHON_ARGS) install
	$(MAKE) -f tclpython.mk $(TCLPYTHON3_ARGS) install

uninstall:
	$(MAKE) -f tclpython.mk $(TCLPYTHON_ARGS) uninstall
	$(MAKE) -f tclpython.mk $(TCLPYTHON3_ARGS) uninstall

clean:
	$(MAKE) -f tclpython.mk $(TCLPYTHON_ARGS) clean
	$(MAKE) -f tclpython.mk $(TCLPYTHON3_ARGS) clean

.PHONY:all test install uninstall clean
