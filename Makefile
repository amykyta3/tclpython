# Override PKG_NAME to tclpython or tclpython3
PKG_NAME=tclpython3
PKG_VERSION=5.1

INSTALL_DIR=/usr/lib/tcltk/x86_64-linux-gnu

#===============================================================================
PYTHON_CONFIG=python3-config
ifeq ($(shell python3 -c "import sys; print(sys.version_info[0:2] >= (3, 8))"),True)
#   Py3.8 and newer require the --embed flag
    PYTHON_CONFIG_LDFLAGS= $(shell $(PYTHON_CONFIG) --ldflags --embed)
else
    PYTHON_CONFIG_LDFLAGS= $(shell $(PYTHON_CONFIG) --ldflags)
endif

BUILD_DIR=build/$(PKG_NAME)
OUTPUT_DIR=$(BUILD_DIR)/$(PKG_NAME)
LIBRARY:= $(PKG_NAME).so.$(PKG_VERSION)

TCL_VERSION=$(shell echo 'puts $\$$tcl_version' | tclsh)
CFLAGS:= -O2 -Wall -fPIC -DUSE_TCL_STUBS
CFLAGS+= $(shell $(PYTHON_CONFIG) --cflags)
CFLAGS+= -I/usr/include/tcl$(TCL_VERSION)
CFLAGS+= -DTCLPYTHON_VERSION=$(PKG_VERSION)
LDFLAGS:= -shared -s
LDFLAGS+= $(PYTHON_CONFIG_LDFLAGS)
LDFLAGS+= -ltclstub$(TCL_VERSION)

SRC:= src/tclpython.c
SRC+= src/py.c

#===============================================================================

all:package

test: package
	TCLLIBPATH=$(OUTPUT_DIR) tclsh test/test.tcl

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

$(OUTPUT_DIR)/pkgIndex.tcl:pkg/pkgIndex.tcl
	cp -t $(dir $@) $^

package: $(OUTPUT_DIR)/$(LIBRARY) $(OUTPUT_DIR)/pkgIndex.tcl

ifneq ($(MAKECMDGOALS), clean)
  -include $(DEPEND)
endif

clean:
	rm -rf build

.PHONY: all test install uninstall clean package
