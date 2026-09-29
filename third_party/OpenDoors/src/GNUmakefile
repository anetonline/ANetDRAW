#  OpenDoors 6.23
#  (C} Copyright 1991 - 1997 by Brian Pirie. All Rights Reserved.
#
#  Oct-2001 door32.sys/socket modifications by Rob Swindell (www.synchro.net}
#
#
#         File: GNUmakefile
#
#  Description: GNU makefile used to build the OpenDoors libraries from the
#               sources. Usage is described below.
#
#    Revisions: Date          Ver   Who  Change
#               ---------------------------------------------------------------
#               Aug 09, 2003  6.23  SH   *nix port
#
###############################################################################
#
# USAGE INFORMATION
#
###############################################################################
#
# Command Line:   gmake
#                     or, where GNU make is installed as make:
#                 make
#
###############################################################################
#
# CONFIGURATION
#
# Customize this section of the makefile to provide the relevant information
# for your compiler, assembler (if any} and build environment.
#
###############################################################################
# Compiler executable file name. Use:
#
#                  tcc - For Borland Turbo C and Turbo C++
#                  bcc - For Borland C++
#                   cl - For Microsoft compilers
#
CC	?=	gcc
AR	?=	ar
RANLIB	?=	ranlib
#
#------------------------------------------------------------------------------
#
# Linker executable file name. Use:
#
#                tlink - For Borland compilers
#                 link - For Microsoft compilers
#
# Get OS name
gcc_machine := $(findstring mingw32,$(shell ${CC} -dumpmachine))
gcc_w64     := $(findstring w64,$(shell ${CC} -dumpmachine))
gcc_x86_64  := $(findstring x86_64,$(shell ${CC} -dumpmachine))
ifeq ($(gcc_machine),mingw32)
 CFLAGS +=      -DMSVCRT_VERSION=0x0800
 ifeq ($(gcc_x86_64),x86_64)
  OS            := Win64
  os            := Win64
  win           := 64
  CFLAGS        += -m64
  LDFLAGS       += -m64
  machine_uname := x64
 else
  OS            := Win32
  os            := Win32
  win           := 32
  CFLAGS        += -m32
  LDFLAGS       += -m32
  WINDRESFLAGS  += -Fpe-i386
  machine_uname := x86
 endif
 ifeq ($(gcc_w64),w64)
  CFLAGS += -DDISABLE_MKSTEMP_DEFINE
 endif
else
 OS      :=      $(shell uname)
 os	:=	$(shell echo $(OS) | tr '[A-Z]' '[a-z]' | tr ' ' '_')
endif
OBJDIR	:=	objs-$(OS)/
LIBDIR	:=	libs-$(OS)/
EXEDIR	:=	exe-$(OS)/
SOURCEDIR := src/
INCLUDEDIR := include/
EXAMPLEDIR := examples/
HEADERDIR := $(SOURCEDIR)

LD	?=	gcc

ifdef DEBUG
 CFLAGS	+=	-g -DOD_DEBUG
 LDFLAGS +=	-g
 BUILDTYPE	:=	debug
else
 BUILDTYPE	:=	release
endif
#
#------------------------------------------------------------------------------
#
# Compiler command-line flags.
#
CFLAGS	+=	-fPIC -fvisibility="hidden"
LDFLAGS	+=	-fPIC
CFLAGS	+=	-O2
ifeq ($(OS),Darwin)
 CFLAGS		+=	-D__unix__
endif
# LDFLAGS links the example programs, not the library (which uses SHFLAGS),
# so shared-library flags do not belong here.
LDFLAGS	+=	-L$(LIBDIR)
# Let examples find the shared library relative to their own output directory.
ifeq ($(OS),Darwin)
 LDFLAGS	+=	-Wl,-rpath,@loader_path/../$(LIBDIR)
else
 ifndef win
  LDFLAGS	+=	'-Wl,-rpath,$$ORIGIN/../$(LIBDIR),-z,origin,--enable-new-dtags'
 endif
endif
CFLAGS	+=	-Wall
CFLAGS += -I$(INCLUDEDIR) -I$(SOURCEDIR) -I.
ifeq ($(shell if [ -f /usr/include/inttypes.h ] ; then echo YES ; fi),YES)
 CFLAGS	+=	-DHAS_INTTYPES_H
endif

# The platforms supported by this GNU makefile provide C99 vsnprintf().
# Set OPENDOORS_HAVE_VSNPRINTF=0 to build the bundled Trio fallback instead.
OPENDOORS_HAVE_VSNPRINTF ?= 1
ifneq ($(OPENDOORS_HAVE_VSNPRINTF),0)
 CFLAGS += -DOPENDOORS_HAVE_VSNPRINTF=1
endif

# /MTd /Zi - for debug
#
#------------------------------------------------------------------------------
#
# Link flags.
#
#
#------------------------------------------------------------------------------
#
# Output directories. customize for your own preferences. Note that trailing
# backslash (\} characters are required.
#
LIB_PREFIX	:=	lib
SHLIB_PREFIX	:=	lib
LIB_SUFFIX	:=	.6.3
EXTRA_LIBS	:=
EXE_SUFFIX	:=
ifeq ($(OPENDOORS_HAVE_VSNPRINTF),0)
 EXTRA_LIBS += -lm
endif
ifeq ($(os),darwin)
 SHLIB		?=	.dylib
 SHFLAGS	+=	-dynamiclib -single_module
 SHFLAGS	+=	-Wl,-install_name,@rpath/$(SHLIB_PREFIX)ODoors$(SHLIB)$(LIB_SUFFIX)
else
 SHFLAGS	+=	-shared
 ifdef win
  SHLIB		?=	.dll
  EXTRA_LIBS += -lwsock32 -lgdi32 -lcomctl32 -Wl,--subsystem,windows
  SHLIB_PREFIX	:=
  LIB_SUFFIX	:=
  EXE_SUFFIX	:= .exe
 else
  SHLIB		?=	.so
  SHFLAGS	+=	-Wl,-Bsymbolic
  SHFLAGS	+=	-Wl,-soname,$(SHLIB_PREFIX)ODoors$(SHLIB)$(LIB_SUFFIX)
 endif
endif
STATICLIB	:=	.a
OBJFILE 	:=	.o
ifdef PROFILE
	CFLAGS	+=	-pg
	SHLIB	:=	_p${SHLIB}
	STATICLIB	:=	_p.a
endif

ODOORS_SHLIB	:= ${LIBDIR}${SHLIB_PREFIX}ODoors${SHLIB}
ODOORS_LIB	:= ${LIBDIR}${LIB_PREFIX}ODoors${STATICLIB}
ifdef win
ODOORS_RUNTIME	:= ${EXEDIR}${SHLIB_PREFIX}ODoors${SHLIB}
EXAMPLE_RUNTIME_DEP := ${ODOORS_RUNTIME}
endif

#
###############################################################################
#
# DEPENDENCIES
#
# You won't normally have to change anything after this point in this makefile.
#
###############################################################################
#
# Define primary target.
#
all: ${OBJDIR} ${LIBDIR} $(EXEDIR) ${ODOORS_SHLIB}${LIB_SUFFIX} \
    ${ODOORS_LIB} $(EXEDIR)ex_chat${EXE_SUFFIX} $(EXEDIR)ex_diag${EXE_SUFFIX} \
    $(EXEDIR)ex_hello${EXE_SUFFIX} $(EXEDIR)ex_music${EXE_SUFFIX} \
    $(EXEDIR)ex_ski${EXE_SUFFIX} $(EXEDIR)ex_vote${EXE_SUFFIX}

.PHONY: static-lib
static-lib: ${ODOORS_LIB}

.PHONY: shared-lib
shared-lib: ${ODOORS_SHLIB}${LIB_SUFFIX}

#
#------------------------------------------------------------------------------
#
# Name of all headers.
#
HEADERS= ${HEADERDIR}ODCom.h\
         ${HEADERDIR}ODCore.h\
         ${HEADERDIR}ODFormat.h\
         ${HEADERDIR}ODGen.h\
         ${HEADERDIR}ODInEx.h\
         ${HEADERDIR}ODInQue.h\
         ${HEADERDIR}ODKrnl.h\
         ${HEADERDIR}ODPlat.h\
         ${HEADERDIR}ODRes.h\
         ${HEADERDIR}ODRsv.h\
         ${HEADERDIR}ODSafe.h\
         ${HEADERDIR}ODScrn.h\
         ${INCLUDEDIR}ODStat.h\
         ${HEADERDIR}ODSwap.h\
         ${HEADERDIR}ODSync.h\
         ${HEADERDIR}ODTypes.h\
         ${HEADERDIR}ODUtil.h\
         ${INCLUDEDIR}OpenDoor.h
#
#------------------------------------------------------------------------------
#
# Build DLL from objects.
#
OBJECTS := ${OBJDIR}ODAuto${OBJFILE}\
         ${OBJDIR}ODBlock${OBJFILE}\
         ${OBJDIR}ODCFile${OBJFILE}\
         ${OBJDIR}ODCmdLn${OBJFILE}\
         ${OBJDIR}ODCom${OBJFILE}\
         ${OBJDIR}ODCore${OBJFILE}\
         ${OBJDIR}ODDrBox${OBJFILE}\
         ${OBJDIR}ODEdit${OBJFILE}\
         ${OBJDIR}ODEdStr${OBJFILE}\
         ${OBJDIR}ODEmu${OBJFILE}\
         ${OBJDIR}ODFormat${OBJFILE}\
         ${OBJDIR}ODFmtFB${OBJFILE}\
         ${OBJDIR}ODGetIn${OBJFILE}\
         ${OBJDIR}ODGraph${OBJFILE}\
         ${OBJDIR}ODInEx1${OBJFILE}\
         ${OBJDIR}ODInEx2${OBJFILE}\
         ${OBJDIR}ODInQue${OBJFILE}\
         ${OBJDIR}ODKrnl${OBJFILE}\
         ${OBJDIR}ODList${OBJFILE}\
         ${OBJDIR}ODLog${OBJFILE}\
         ${OBJDIR}ODMulti${OBJFILE}\
         ${OBJDIR}ODPlat${OBJFILE}\
         ${OBJDIR}ODPCB${OBJFILE}\
         ${OBJDIR}ODPopup${OBJFILE}\
         ${OBJDIR}ODPrntf${OBJFILE}\
         ${OBJDIR}ODRA${OBJFILE}\
         ${OBJDIR}ODRsv${OBJFILE}\
         ${OBJDIR}ODSafe${OBJFILE}\
         ${OBJDIR}ODScrn${OBJFILE}\
         ${OBJDIR}ODSpawn${OBJFILE}\
         ${OBJDIR}ODStand${OBJFILE}\
         ${OBJDIR}ODStat${OBJFILE}\
         ${OBJDIR}ODStr${OBJFILE}\
         ${OBJDIR}ODSync${OBJFILE}\
         ${OBJDIR}ODUtil${OBJFILE}\
         ${OBJDIR}ODWCat${OBJFILE}\
	 ${OBJDIR}ODWin${OBJFILE}\
#         ${OBJDIR}odsys${OBJFILE}\	this file is missing

ifdef win
	OBJECTS += ${OBJDIR}ODFrame${OBJFILE} ${LIBDIR}ODoor.res
endif

${OBJDIR}:
	mkdir ${OBJDIR}

${LIBDIR}:
	mkdir ${LIBDIR}

${EXEDIR}:
	mkdir ${EXEDIR}

$(OBJDIR)%$(OBJFILE) : $(SOURCEDIR)%.c |  ${OBJDIR}
	$(CC) $(CFLAGS) -o $@ -c $<

$(LIBDIR)ODoor.res : $(SOURCEDIR)ODRes.rc | ${LIBDIR}
	$(WINDRES) $(WINDRESFLAGS) -I $(SOURCEDIR) -O coff -i $< -o $@

${ODOORS_SHLIB}${LIB_SUFFIX} : ${OBJECTS} | ${LIBDIR}
	$(CC) $(SHFLAGS) -o ${ODOORS_SHLIB}${LIB_SUFFIX} ${OBJECTS} ${EXTRA_LIBS}

ifdef win
${ODOORS_RUNTIME}: ${ODOORS_SHLIB} | ${EXEDIR}
	cp -f $< $@
endif

ifndef win
${ODOORS_SHLIB}: ${ODOORS_SHLIB}${LIB_SUFFIX}
	ln -fs ${SHLIB_PREFIX}ODoors${SHLIB}${LIB_SUFFIX} ${ODOORS_SHLIB}
endif

${ODOORS_LIB} : ${OBJECTS} | ${LIBDIR}
	${AR} -cr ${ODOORS_LIB} ${OBJECTS}
	${RANLIB} ${ODOORS_LIB}

include examples/GNUmakefile

#
#------------------------------------------------------------------------------
