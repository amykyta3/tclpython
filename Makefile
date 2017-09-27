# Override PKG_NAME to tclpython or tclpython3
PKG_NAME=tclpython
PKG_VERSION=4.2

INSTALL_DIR=/usr/lib/tcltk/x86_64-linux-gnu

#===============================================================================
ifeq ($(PKG_NAME),tclpython)
	PYTHON_CONFIG=python2-config
else ifeq ($(PKG_NAME),tclpython3)
	PYTHON_CONFIG=python3-config
endif

BUILD_DIR=build_$(PKG_NAME)
OUTPUT_DIR=$(BUILD_DIR)/$(PKG_NAME)
LIBRARY:= $(PKG_NAME).so.$(PKG_VERSION)

TCL_VERSION=$(shell echo 'puts $\$$tcl_version' | tclsh)
CFLAGS:= -O2 -Wall -fPIC -DUSE_TCL_STUBS
CFLAGS+= $(shell $(PYTHON_CONFIG) --includes)
CFLAGS+= -I/usr/include/tcl$(TCL_VERSION)
LDFLAGS:= -shared -s
LDFLAGS+= $(shell $(PYTHON_CONFIG) --libs)
LDFLAGS+= -ltclstub$(TCL_VERSION)

SRC:= src/tclpython.c
SRC+= src/tclthread.c

#===============================================================================

all:package

test: package
	TCLLIBPATH=$(OUTPUT_DIR) tclsh test/test.tcl $(PKG_NAME)

install: package
	cp -r $(OUTPUT_DIR) $(INSTALL_DIR)/$(PKG_NAME)

uninstall:
	rm -rf $(INSTALL_DIR)/$(PKG_NAME)

#===============================================================================
OBJECTS:= $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(basename $(SRC))))
DEPEND:= $(OBJECTS:.o=.d)

# Generate Dependencies
$(BUILD_DIR)/%.d: %.c
	@mkdir -p $(dir $@)
	@$(CC) -MM -MT $(@:.d=.o) -MT $@ $(CFLAGS) $< >$@

# C Compile
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	
# Link
$(OUTPUT_DIR)/$(LIBRARY): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^ $(LDFLAGS)

$(OUTPUT_DIR)/pkgIndex.tcl:pkg/$(PKG_NAME)/pkgIndex.tcl
	cp -t $(dir $@) $^
	
package: $(OUTPUT_DIR)/$(LIBRARY) $(OUTPUT_DIR)/pkgIndex.tcl

ifneq ($(MAKECMDGOALS), clean)
  -include $(DEPEND)
endif

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all test install uninstall clean package
