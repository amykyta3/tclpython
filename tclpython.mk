
#===============================================================================
PYTHON_VERSION=2.7
#PYTHON_VERSION=3.5m

INSTALL_DIR=/usr/lib/tcltk/x86_64-linux-gnu

TCL_PKG_NAME=tclpython
#TCL_PKG_NAME=tclpython3

TCL_PKG_VERSION=4.1

#===============================================================================
TCL_VERSION=$(shell echo 'puts $\$$tcl_version' | tclsh)

BUILD_DIR=build_$(TCL_PKG_NAME)
OUTPUT_DIR=$(BUILD_DIR)/$(TCL_PKG_NAME)
LIBRARY:= $(TCL_PKG_NAME).so.$(TCL_PKG_VERSION)
INCLUDES:= /usr/include/python$(PYTHON_VERSION) /usr/include/tcl$(TCL_VERSION)
CFLAGS:= -O2 -Wall -fPIC -DUSE_TCL_STUBS
LDFLAGS:= -shared -s
LDFLAGS+= -lpython$(PYTHON_VERSION)
LDFLAGS+= -lpthread -lutil
LDFLAGS+= -ltclstub$(TCL_VERSION)

SRC:= src/tclpython.c
SRC+= src/tclthread.c

PKG_FILES:=pkgIndex.tcl
#===============================================================================

all:package

test: package
	TCLLIBPATH=$(OUTPUT_DIR) tclsh test/$(TCL_PKG_NAME)_test.tcl

install: package
	cp -r $(OUTPUT_DIR) $(INSTALL_DIR)/$(TCL_PKG_NAME)

uninstall:
	rm -rf $(INSTALL_DIR)/$(TCL_PKG_NAME)

#===============================================================================
OBJECTS:= $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(basename $(SRC))))
DEPEND:= $(OBJECTS:.o=.d)

# Generate Dependencies
$(BUILD_DIR)/%.d: %.c
	@mkdir -p $(dir $@)
	@$(CC) -MM -MT $(@:.d=.o) -MT $@  $(addprefix -I,$(INCLUDES)) $(CFLAGS) $< >$@

# C Compile
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(addprefix -I,$(INCLUDES)) $(CFLAGS) -c -o $@ $<
	
# Link
$(OUTPUT_DIR)/$(LIBRARY): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OUTPUT_DIR)/$(PKG_FILES):pkg/$(TCL_PKG_NAME)/$(PKG_FILES)
	cp -t $(dir $@) $^
	
package: $(OUTPUT_DIR)/$(LIBRARY) $(OUTPUT_DIR)/$(PKG_FILES)

ifneq ($(MAKECMDGOALS), clean)
  -include $(DEPEND)
endif

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test install uninstall clean package
