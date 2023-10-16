#
# SDK Makefile for x86 Linux
#
#

OS := $(shell uname -s)

#############################################################################
# Developer configurable items
#############################################################################

# the name of the mod binary (_i486.so is appended to the end)
NAME = server

# the location of the vcproj that builds the mod
MOD_PROJ = ../game/server/server_scratch-2005.vcproj
# the name of the mod configuration (typically <proj name>_<build type><build target>)
MOD_CONFIG = Server\(SDK\)_ReleaseWin32

# the directory the base binaries (tier0_i486.so, etc) are located
# this should point to your orange box subfolder of where you have srcds installed.
SRCDS_DIR = ~/srcds/orangebox

# the path to your mods directory
# set this so that 'make install' or 'make installrelease' will copy your binary over automatically.
GAME_DIR = $(SRCDS_DIR)/scratchmod

CC = /usr/bin/clang
CPLUS = /usr/bin/clang++
CLINK = /usr/bin/clang
ifeq "$(OS)" "Darwin"
CPP_LIB =
else
CPP_LIB = "$(SRCDS_DIR)/bin/libstdc++.so.6 $(SRCDS_DIR)/bin/libgcc_s.so.1"
endif

# put any compiler flags you want passed here
USER_CFLAGS =

# link flags for your mod, make sure to include any special libraries here
LDFLAGS = "-lm -ldl $(LIB_DIR)/particles.a $(LIB_DIR)/dmxloader.a $(LIB_DIR)/mathlib.a libtier0.so libvstdlib.so $(LIB_DIR)/tier1.a $(LIB_DIR)/tier2.a $(LIB_DIR)/tier3.a $(LIB_DIR)/choreoobjects.a libsteam_api64.so"

# XERCES 2.6.0 or above ( http://xml.apache.org/xerces-c/ ) is used by the vcproj to makefile converter
# it must be installed before being able to run this makefile
# if you have xerces installed already you should be able to use the two lines below
XERCES_INC_DIR = /usr/include
XERCES_LIB_DIR = /usr/lib

# Change this to true if you want to build debug binaries for everything
# The only exception is the mod/game as MOD_CONFIG determines if it's a debug build or not
DEBUG = false

#############################################################################
# Things below here shouldn't need to be altered
#############################################################################
IS_CLANG := $(shell $(CPP) --version | head -1 | grep clang > /dev/null && echo "1" || echo "0")

ifeq "$(IS_CLANG)" "1"
CPP_MAJOR := $(shell $(CPP) --version | grep clang | sed "s/.*version \([0-9]\)*\.[0-9]*.*/\1/")
CPP_MINOR := $(shell $(CPP) --version | grep clang | sed "s/.*version [0-9]*\.\([0-9]\)*.*/\1/")
else
CPP_MAJOR := $(shell $(CPP) -dumpversion >&1 | cut -b1)
CPP_MINOR := $(shell $(CPP) -dumpversion >&1 | cut -b3)
endif

MAKE = make
AR = "ar rvs"

# the dir we want to put binaries we build into
BUILD_DIR = .
# the place to put object files
BUILD_OBJ_DIR = $(BUILD_DIR)/obj

# the location of the source code
SRC_DIR = ..

# the location of the static libraries
ifeq "$(OS)" "Darwin"
LIB_DIR = $(SRC_DIR)/lib/mac
else
LIB_DIR = $(SRC_DIR)/lib/linux64
endif

# the CPU target for the build, must be i486 for now
ARCH = i486
ARCH_CFLAGS = -march=nocona -mmmx -msse -msse2 -m64

ifeq "$(OS)" "Darwin"
DEFINES = -D_OSX -DOSX -D_DLL_EXT=.dylib
ARCH_CFLAGS += -mmacosx-version-min=10.7
SHLIBEXT = dylib
SHLIBLDFLAGS = -dynamiclib -mmacosx-version-min=10.7
SHLIBSUFFIX =
else
DEFINES = -D_LINUX -DLINUX -D_DLL_EXT=.so
SHLIBEXT = so
SHLIBLDFLAGS = -shared -Wl,-Map,$@_map.txt
SHLIBSUFFIX =
endif

DEFINES +=-DX64BITS -DPLATFORM_64BITS -DVPROF_LEVEL=1 -DSWDS -D_finite=finite -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp \
	-Dstrnicmp=strncasecmp -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp -DPOSIX -DGNUC -DCOMPILER_GCC -DNO_MALLOC_OVERRIDE
UNDEF = -Usprintf -Ustrncpy -UPROTECTED_THINGS_ENABLE

BASE_CFLAGS = -fPIC -fno-strict-aliasing -Wall -Wsign-compare -Wno-conversion -Wno-overloaded-virtual -Wno-non-virtual-dtor -Wno-invalid-offsetof \
	      -Wno-unknown-pragmas -Wno-unused -Wno-c++11-narrowing
SHLIBCFLAGS = -fPIC

# Clang >= 3 || GCC >= 4.7
ifeq "$(shell expr $(IS_CLANG) \& $(CPP_MAJOR) \>= 3 \| $(CPP_MAJOR) \>= 4 \& $(CPP_MINOR) \>= 7)" "1"
BASE_CFLAGS += -Wno-delete-non-virtual-dtor -Wno-narrowing
endif

# Flags passed to the c compiler
CFLAGS = $(DEFINES) $(ARCH_CFLAGS) -O3 $(BASE_CFLAGS)
ifdef USER_CFLAGS
	CFLAGS += $(USER_CFLAGS)
endif
CFLAGS += $(UNDEF)

# Debug flags
DBG_DEFINES = "-D_DEBUG -DDEBUG"
DBG_CFLAGS = "$(DEFINES) $(ARCH_CFLAGS) -g -ggdb $(BASE_CFLAGS) $(UNDEF)"

# define list passed to make for the sub makefile
BASE_DEFINES = CC=$(CC) AR=$(AR) CPLUS=$(CPLUS) CPP_LIB=$(CPP_LIB) DEBUG=$(DEBUG) \
	BUILD_DIR=$(BUILD_DIR) BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) SRC_DIR=$(SRC_DIR) \
	LIB_DIR=$(LIB_DIR) SHLIBLDFLAGS="$(SHLIBLDFLAGS)" SHLIBEXT=$(SHLIBEXT) SHLIBSUFFIX=$(SHLIBSUFFIX) \
	CLINK=$(CLINK) CFLAGS="$(CFLAGS)" DBG_CFLAGS=$(DBG_CFLAGS) LDFLAGS=$(LDFLAGS) \
	DEFINES="$(DEFINES)" DBG_DEFINES=$(DBG_DEFINES) \
	ARCH=$(ARCH) SRCDS_DIR=$(SRCDS_DIR) MOD_CONFIG=$(MOD_CONFIG) NAME=$(NAME) \
	XERCES_INC_DIR=$(XERCES_INC_DIR) XERCES_LIB_DIR=$(XERCES_LIB_DIR)

# Project Makefile
MAKE_SERVER = Makefile.server
MAKE_VCPM = Makefile.vcpm
MAKE_SRCDS = Makefile.srcds
MAKE_PLUGIN = Makefile.plugin
MAKE_TIER1 = Makefile.tier1
MAKE_MATH = Makefile.mathlib
MAKE_IFACE = Makefile.interfaces
MAKE_CHOREO = Makefile.choreo

all: check vcpm mod

check:
	if [ -z "$(CC)" ]; then echo "Compiler not defined."; exit; fi
	if [ ! -d $(BUILD_DIR) ];then mkdir -p $(BUILD_DIR);fi
	cd $(BUILD_DIR)
	if [ ! -e "$(LIB_DIR)/tier1.a" ]; then $(MAKE) tier1;fi
	if [ ! -e "$(LIB_DIR)/mathlib.a" ]; then $(MAKE) mathlib;fi
	if [ ! -e "$(LIB_DIR)/interfaces.a" ]; then $(MAKE) interfaces;fi
	if [ ! -f "libtier0$(SHLIBSUFFIX).$(SHLIBEXT)" ]; then ln -fs $(LIB_DIR)/libtier0$(SHLIBSUFFIX).$(SHLIBEXT) .; fi
	if [ ! -f "libvstdlib$(SHLIBSUFFIX).$(SHLIBEXT)" ]; then ln -fs $(LIB_DIR)/libvstdlib$(SHLIBSUFFIX).$(SHLIBEXT) .; fi
	if [ ! -f "libsteam_api64.$(SHLIBEXT)" ]; then ln -fs $(LIB_DIR)/libsteam64_api.$(SHLIBEXT) .; fi

vcpm: check
	if [ ! -e "vcpm" ]; then $(MAKE) -f $(MAKE_VCPM) $(BASE_DEFINES);fi

mod: check vcpm
	./vcpm $(MOD_PROJ)
	$(MAKE) -f $(MAKE_SERVER) $(BASE_DEFINES)

srcds:
	$(MAKE) -f $(MAKE_SRCDS) $(BASE_DEFINES)

plugin: check
	$(MAKE) -f $(MAKE_PLUGIN) $(BASE_DEFINES)
	
tier1:
	$(MAKE) -f $(MAKE_TIER1) $(BASE_DEFINES)

mathlib:
	$(MAKE) -f $(MAKE_MATH) $(BASE_DEFINES)

interfaces:
	$(MAKE) -f $(MAKE_IFACE) $(BASE_DEFINES)

choreo:
	$(MAKE) -f $(MAKE_CHOREO) $(BASE_DEFINES)

install:
	cp -f $(NAME).$(SHLIBEXT) $(GAME_DIR)/bin/$(NAME).$(SHLIBEXT)

installrelease:
	cp -f $(NAME).$(SHLIBEXT) $(GAME_DIR)/bin/$(NAME).$(SHLIBEXT)
	strip $(GAME_DIR)/bin/$(NAME).$(SHLIBEXT)

clean:
	$(MAKE) -f $(MAKE_VCPM) $(BASE_DEFINES) clean
	$(MAKE) -f $(MAKE_SRCDS) $(BASE_DEFINES) clean
	$(MAKE) -f $(MAKE_PLUGIN) $(BASE_DEFINES) clean
	$(MAKE) -f $(MAKE_SERVER) $(BASE_DEFINES) clean
	$(MAKE) -f $(MAKE_TIER1) $(BASE_DEFINES) clean
	$(MAKE) -f $(MAKE_MATH) $(BASE_DEFINES) clean
	$(MAKE) -f $(MAKE_IFACE) $(BASE_DEFINES) clean
	$(MAKE) -f $(MAKE_CHOREO) $(BASE_DEFINES) clean
