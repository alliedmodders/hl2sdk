#
# SDK Makefile for x86 Linux
#
#

#############################################################################
# Developer configurable items
#############################################################################

# the name of the mod binary (_i486.so is appended to the end)
NAME=server

# the location of the vcproj that builds the mod
MOD_PROJ=../game/server/server_scratch-2005.vcproj
# the name of the mod configuration (typically <proj name>_<build type><build target>)
MOD_CONFIG=ServerSDK_ReleaseWin32

# the directory the base binaries (tier0_i486.so, etc) are located
# this should point to your orange box subfolder of where you have srcds installed.
GAME_DIR=~/srcds/orangebox

# the path to your mods directory
# set this so that 'make install' or 'make installrelease' will copy your binary over automatically.
MOD_DIR=$(GAME_DIR)/scratchmod

# compiler options (gcc 3.4.1 or above is required - 4.1.2+ recommended)
CC=/usr/local/bin/gcc
CPLUS=/usr/local/bin/g++
CLINK=/usr/local/bin/gcc
CPP_LIB=" /usr/local/lib/libstdc++.a /usr/local/lib/libgcc_eh.a"

# put any compiler flags you want passed here
USER_CFLAGS=

# link flags for your mod, make sure to include any special libraries here
LDFLAGS="-lm -ldl $(SOURCE_DIR)/lib/linux/particles_486.a $(SOURCE_DIR)/lib/linux/dmxloader_486.a $(SOURCE_DIR)/lib/linux/mathlib_486.a tier0_i486.so vstdlib_i486.so $(SOURCE_DIR)/lib/linux/tier1_486.a $(SOURCE_DIR)/lib/linux/tier2_486.a $(SOURCE_DIR)/lib/linux/tier3_486.a $(SOURCE_DIR)/lib/linux/choreoobjects_486.a steam_api_i486.so"

# XERCES 2.6.0 or above ( http://xml.apache.org/xerces-c/ ) is used by the vcproj to makefile converter
# it must be installed before being able to run this makefile
# if you have xerces installed already you should be able to use the two lines below
XERCES_INC_DIR=/usr/include
XERCES_LIB_DIR=/usr/lib


#############################################################################
# Things below here shouldn't need to be altered
#############################################################################
MAKE=make

# the dir we want to put binaries we build into
BUILD_DIR=.
# the place to put object files
BUILD_OBJ_DIR=$(BUILD_DIR)/obj

# the location of the source code
SOURCE_DIR=..

# the CPU target for the build, must be i486 for now
ARCH=i486
ARCH_CFLAGS=-mtune=i686 -march=pentium3 -mmmx -O3

# -fpermissive is so gcc 3.4.x doesn't complain about some template stuff
BASE_CFLAGS=-DVPROF_LEVEL=1 -DSWDS -D_LINUX -DLINUX -DNDEBUG -fpermissive -Dstricmp=strcasecmp -D_stricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf -D_alloca=alloca -Dstrcmpi=strcasecmp
SHLIBEXT=so
SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS=-shared -Wl,-Map,$@_map.txt -Wl

#flags passed to the c compiler
#CFLAGS="$(USER_CFLAGS) $(DEFINES) $(ARCH_CFLAGS) $(OPTIMIZER_FLAGS) $(BASE_CFLAGS) -Dsprintf=use_Q_snprintf_instead_of_sprintf -Dstrncpy=use_Q_strncpy_instead -D_snprintf=use_Q_snprintf_instead -UPROTECTED_THINGS_ENABLE"
CFLAGS="$(USER_CFLAGS) $(DEFINES) $(ARCH_CFLAGS) $(BASE_CFLAGS) -Usprintf=use_Q_snprintf_instead_of_sprintf -Ustrncpy=use_Q_strncpy_instead -UPROTECTED_THINGS_ENABLE"

# define list passed to make for the sub makefile
BASE_DEFINES=CC=$(CC) CPLUS=$(CPLUS) CPP_LIB=$(CPP_LIB) \
	BUILD_DIR=$(BUILD_DIR) BUILD_OBJ_DIR=$(BUILD_OBJ_DIR) \
	SOURCE_DIR=$(SOURCE_DIR) SHLIBLDFLAGS=$(SHLIBLDFLAGS) SHLIBEXT=$(SHLIBEXT) \
	CLINK=$(CLINK) CFLAGS=$(CFLAGS) LDFLAGS=$(LDFLAGS) \
	ARCH=$(ARCH) GAME_DIR=$(GAME_DIR) MOD_CONFIG=$(MOD_CONFIG) NAME=$(NAME) \
	XERCES_INC_DIR=$(XERCES_INC_DIR) XERCES_LIB_DIR=$(XERCES_LIB_DIR)

# Project Makefile
MAKE_MOD=Makefile.server
MAKE_VCPM=Makefile.vcpm
MAKE_PLUGIN=Makefile.plugin

all: check vcpm server

check:
	if [ -z "$(CC)" ]; then echo "Compiler not defined."; exit; fi
	if [ ! -d $(BUILD_DIR) ];then mkdir $(BUILD_DIR);fi
	cd $(BUILD_DIR)

vcpm:
	$(MAKE) -f $(MAKE_VCPM) $(BASE_DEFINES)

server: vcpm
	if [ ! -f "tier0_i486.so" ]; then ln -s $(GAME_DIR)/bin/tier0_i486.so .; fi
	if [ ! -f "vstdlib_i486.so" ]; then ln -s $(GAME_DIR)/bin/vstdlib_i486.so .; fi
	if [ ! -f "steam_api_i486.so" ]; then ln -s $(GAME_DIR)/bin/steam_api_i486.so .; fi
# When running over samba we need to copy the files because symlinking isn't possible.
#	cp -f $(GAME_DIR)/bin/tier0_i486.so .
#	cp -f $(GAME_DIR)/bin/vstdlib_i486.so .
#	cp -f $(GAME_DIR)/bin/steam_api_i486.so .

	./vcpm $(MOD_PROJ)
	$(MAKE) -f $(MAKE_MOD) $(BASE_DEFINES)

plugin:
	$(MAKE) -f $(MAKE_PLUGIN) $(BASE_DEFINES)

clean:
	 $(MAKE) -f $(MAKE_VCPM) $(BASE_DEFINES) clean
	 $(MAKE) -f $(MAKE_PLUGIN) $(BASE_DEFINES) clean
	 $(MAKE) -f $(MAKE_MOD) $(BASE_DEFINES) clean

install:
	cp -f $(NAME)_$(ARCH).so $(MOD_DIR)/bin/$(NAME)_$(ARCH).so

installrelease:
	cp -f $(NAME)_$(ARCH).so $(MOD_DIR)/bin/$(NAME)_$(ARCH).so
	strip $(MOD_DIR)/bin/$(NAME)_$(ARCH).so
