
PYTHON_VERSION=python2.7

PROJECT_NAME:=application
BUILD_PATH:=build/


TCL_VERSION=$(shell echo 'puts $\$$tcl_version' | tclsh)

### TODO. Auto-discover these paths better
TCLSTUB_LIB=/usr/lib/x86_64-linux-gnu/libtclstub$(TCL_VERSION).a
INSTALL_DIR=/usr/lib/tcltk/x86_64-linux-gnu
PYTHON_LIB_PATH=/usr/lib/$(PYTHON_VERSION)/config-x86_64-linux-gnu
####


TCL_PKG_NAME=tclpython
LIBRARY:= $(TCL_PKG_NAME).so.4.1

INCLUDES:= /usr/include/$(PYTHON_VERSION) /usr/include/tcl$(TCL_VERSION)
CFLAGS:= -O2 -Wall -fPIC
LDFLAGS:= -shared -s
LDFLAGS+= -L$(PYTHON_LIB_PATH) -l$(PYTHON_VERSION)
LDFLAGS+= -lpthread -lutil
LDFLAGS+= $(TCLSTUB_LIB)


SRC:= src/tclpython.c
SRC+= src/tclthread.c

#===============================================================================

all:$(LIBRARY)
	
install: $(LIBRARY)
	mkdir -p $(INSTALL_DIR)/$(TCL_PKG_NAME)
	cp $(LIBRARY) $(INSTALL_DIR)/$(TCL_PKG_NAME)
	cp pkg/pkgIndex.tcl $(INSTALL_DIR)/$(TCL_PKG_NAME)

uninstall:
	rm -rf $(INSTALL_DIR)/$(TCL_PKG_NAME)

#===============================================================================
OBJECTS:= $(addsuffix .o,$(basename $(SRC)))
DEPEND:= $(OBJECTS:.o=.d)

# Generate Dependencies
%.d: %.c
	@mkdir -p $(dir $@)
	@$(CC) -MM -MT $(@:.d=.o) -MT $@  $(addprefix -I,$(INCLUDES)) $(CFLAGS) $< >$@

# C Compile
%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(addprefix -I,$(INCLUDES)) $(CFLAGS) -c -o $@ $<
	
# Link
$(LIBRARY): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^ $(LDFLAGS)
	
ifneq ($(MAKECMDGOALS), clean)
  -include $(DEPEND)
endif

clean:
	@rm -r -f $(OBJECTS) $(DEPEND) $(LIBRARY)
