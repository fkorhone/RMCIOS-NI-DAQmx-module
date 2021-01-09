include RMCIOS-build-scripts/utilities.mk

SOURCES:=*.c
FILENAME:=nidaqmx-module
GCC?=${TOOL_PREFIX}gcc
DLLTOOL?=${TOOL_PREFIX}dlltool
MAKE?=make
INSTALLDIR:=..${/}..
LINKDEF?=NIDAQmx.def
CFLAGS+=libnidaqmx.a 
CFLAGS+=-I./linklib
export

compile:
	$(DLLTOOL) -k -d ./linklib/$(LINKDEF) -l libnidaqmx.a 
	$(MAKE) -f RMCIOS-build-scripts${/}module_dll.mk compile TOOL_PREFIX=${TOOL_PREFIX}

install:
	-${MKDIR} "${INSTALLDIR}${/}modules"
	${COPY} *.dll ${INSTALLDIR}${/}modules

