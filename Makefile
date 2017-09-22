
#===============================================================================
#PYTHON_VERSION=2.7
PYTHON_VERSION=3.5m
INSTALL_DIR=/usr/lib/tcltk/x86_64-linux-gnu

TCL_PKG_NAME=tclpython
TCL_PKG_VERSION=4.1

#===============================================================================
TCL_VERSION=$(shell echo 'puts $\$$tcl_version' | tclsh)

OUTPUT_DIR=pkg/
LIBRARY:= $(TCL_PKG_NAME).so.$(TCL_PKG_VERSION)
INCLUDES:= /usr/include/python$(PYTHON_VERSION) /usr/include/tcl$(TCL_VERSION)
CFLAGS:= -O2 -Wall -fPIC -DUSE_TCL_STUBS
LDFLAGS:= -shared -s
LDFLAGS+= -lpython$(PYTHON_VERSION)
LDFLAGS+= -lpthread -lutil
LDFLAGS+= -ltclstub$(TCL_VERSION)

SRC:= src/tclpython.c
SRC+= src/tclthread.c

#===============================================================================

all:$(OUTPUT_DIR)$(LIBRARY)

test: $(OUTPUT_DIR)$(LIBRARY)
	TCLLIBPATH=$(OUTPUT_DIR) tclsh test/test.tcl

install: $(OUTPUT_DIR)$(LIBRARY)
	cp -r $(OUTPUT_DIR) $(INSTALL_DIR)/$(TCL_PKG_NAME)

uninstall:
	rm -rf $(INSTALL_DIR)/$(TCL_PKG_NAME)

#===============================================================================
OBJECTS:= $(addsuffix .o,$(basename $(SRC)))
DEPEND:= $(OBJECTS:.o=.d)

# Generate Dependencies
%.d: %.c
	@$(CC) -MM -MT $(@:.d=.o) -MT $@  $(addprefix -I,$(INCLUDES)) $(CFLAGS) $< >$@

# C Compile
%.o: %.c
	$(CC) $(addprefix -I,$(INCLUDES)) $(CFLAGS) -c -o $@ $<
	
# Link
$(OUTPUT_DIR)$(LIBRARY): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^ $(LDFLAGS)
	
ifneq ($(MAKECMDGOALS), clean)
  -include $(DEPEND)
endif

clean:
	rm -rf $(OBJECTS) $(DEPEND) $(OUTPUT_DIR)$(LIBRARY)

.PHONY: all test install uninstall clean
