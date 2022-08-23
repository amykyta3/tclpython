
# Setup Paths
TCL_PATH=C:\Tcl
PYTHON2_PATH=C:\Python27
PYTHON3_PATH=C:\Program Files (x86)\Python36-32
INSTALL_DIR=C:\Tcl\lib\tcl8.5

# Override PKG_NAME to tclpython or tclpython3
PKG_NAME=tclpython3
PKG_VERSION=5.0

#===============================================================================
!if ("$(PKG_NAME)" == "tclpython")
PYTHON_PATH=$(PYTHON2_PATH)
!else if ("$(PKG_NAME)" == "tclpython3")
PYTHON_PATH=$(PYTHON3_PATH)
!endif

BUILD_DIR=build\$(PKG_NAME)
OUTPUT_DIR=$(BUILD_DIR)\$(PKG_NAME)
LIBRARY= $(PKG_NAME)-$(PKG_VERSION).dll

CFLAGS= -nologo -W3 -MT -EHsc -FD -O2 -Gs \
		-D "WIN32" -D "NDEBUG" -D "_WINDOWS" -D "_MBCS" \
		-D "_USRDLL" -D "USE_TCL_STUBS" \
		-DTCLPYTHON_VERSION=$(PKG_VERSION) \
		-I"$(TCL_PATH)\include" \
		-I"$(PYTHON_PATH)\include"

LDFLAGS= -nologo  -release -opt:ref -opt:icf,3 -dll \
		-incremental:no -nodefaultlib:MSVCRT.LIB \
		-libpath:"$(TCL_PATH)\lib" \
		-libpath:"$(PYTHON_PATH)\libs"

OBJECTS= $(BUILD_DIR)/tclpython.obj $(BUILD_DIR)/py.obj

#===============================================================================

package:$(OUTPUT_DIR)/$(LIBRARY) $(OUTPUT_DIR)\pkgIndex.tcl

test: package
	@test_wrapper.bat "$(TCL_PATH)" "$(OUTPUT_DIR)" $(PKG_NAME) "$(PYTHON_PATH)"

install: package
	xcopy /Y /I $(OUTPUT_DIR) $(INSTALL_DIR)\$(PKG_NAME)

uninstall:
	-rmdir /S /Q $(INSTALL_DIR)\$(PKG_NAME)

#===============================================================================

# C Compile
{..\src\}.c{$(BUILD_DIR)}.obj:
	@echo "$< --> $@"
	@-mkdir $(@D)
	$(CC) $(CFLAGS) -c -Fo$@ $<

# Link
$(OUTPUT_DIR)\$(LIBRARY): $(TCL_PATH)\lib\tclstub*.lib $(OBJECTS)
	@echo "$? --> $@"
	@-mkdir $(@D)
	link $(LDFLAGS) -out:$@ $?

$(OUTPUT_DIR)\pkgIndex.tcl :..\pkg\pkgIndex.tcl
	copy ..\pkg\pkgIndex.tcl $(OUTPUT_DIR)\pkgIndex.tcl

clean:
	-rmdir /S /Q build
