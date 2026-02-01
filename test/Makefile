# Copyright 2026, Stephen Fryatt (info@stevefryatt.org.uk)
#
# This file is part of Locate:
#
#   http://www.stevefryatt.org.uk/software/
#
# Licensed under the EUPL, Version 1.2 only (the "Licence");
# You may not use this work except in compliance with the
# Licence.
#
# You may obtain a copy of the Licence at:
#
#   http://joinup.ec.europa.eu/software/page/eupl
#
# Unless required by applicable law or agreed to in
# writing, software distributed under the Licence is
# distributed on an "AS IS" basis, WITHOUT WARRANTIES
# OR CONDITIONS OF ANY KIND, either express or implied.
#
# See the Licence for the specific language governing
# permissions and limitations under the Licence.

# Unit Test makefile to generate Unit Tests for some Locate modules using
# Unity: see https://www.throwtheswitch.org/ for more details.
#
# This file really needs to be run by GNUMake.
# It is intended for native compilation on Linux (for use in a GCCSDK
# environment) or cross-compilation under the GCCSDK.

# The file is based on the Unity Makefile described at
# https://www.throwtheswitch.org/build/make


# The objects from Locate which are required to build any of the tests.
# This should include the targets, and any of the other modules included
# by those targets.

OBJECTS = ignore.o textdump.o

# Configure for building on Linux but running the executables on RISC OS.

CLEANUP = rm -f
MKDIR = mkdir -p
TARGET_EXTENSION=,ff8

CC := $(wildcard $(GCCSDK_INSTALL_CROSSBIN)/*gcc)
CCFLAGS=-std=gnu99 -mlibscl -mhard-float -static -mthrowback -Wall -O2 -fno-strict-aliasing -mpoke-function-name

COMPILE=$(CC) -c
LINK=$(CC)
DEPEND=$(CC) -MM -MG -MF

.PHONY: clean
.PHONY: test

PATHU = unity/
PATHS = ../src/
PATHT = tests/
PATHB = absolute/
PATHO = ../obj/
PATHD = $(PATHO)

BUILD_PATHS = $(PATHB) $(PATHO)

TESTPREFIX = test_
TARGETPREFIX = target_

SRCT = $(wildcard $(PATHT)*.c)

EXECUTABLES = $(patsubst $(PATHT)$(TESTPREFIX)%.c,$(PATHB)$(TESTPREFIX)%$(TARGET_EXTENSION),$(SRCT) )

OBJECTS := $(addprefix $(PATHO)$(TESTPREFIX)$(TARGETPREFIX), $(OBJECTS))

# The libraries.

INCLUDES=-I. -I$(PATHU) -I$(PATHS) -I${GCCSDK_INSTALL_ENV}/include
LINKS=-L${GCCSDK_INSTALL_ENV}/lib -lOSLibH32 -lSFLib32 -lFlexLib32
DEFINES=-DUNIT_TESTING

test: $(BUILD_PATHS) $(EXECUTABLES)

$(PATHB)$(TESTPREFIX)%$(TARGET_EXTENSION): $(PATHO)$(TESTPREFIX)%.o $(PATHO)unity.o $(OBJECTS) #$(PATHD)$(TESTPREFIX)%.d
	$(LINK) $(CCFLAGS) $(LINKS) -o $@ $^

# Make the test object files.

$(PATHO)%.o:: $(PATHT)%.c
	$(COMPILE) $(CCFLAGS) $(INCLUDES) $(DEFINES) $< -o $@

# Make the target object files.

$(PATHO)$(TESTPREFIX)$(TARGETPREFIX)%.o:: $(PATHS)%.c
	$(COMPILE) $(CCFLAGS) $(INCLUDES) $(DEFINES) $< -o $@

# Make the Unity object files.

$(PATHO)%.o:: $(PATHU)%.c $(PATHU)%.h
	$(COMPILE) $(CCFLAGS) $(INCLUDES) $(DEFINES) $< -o $@

# Make the dependency lists.

$(PATHD)%.d:: $(PATHT)%.c
	$(DEPEND) $(CCFLAGS) $(INCLUDES) $(DEFINES) $@ $<

# Create the necessary output folders.

$(PATHB):
	$(MKDIR) $(PATHB)

$(PATHO):
	$(MKDIR) $(PATHO)

clean:
	$(CLEANUP) $(PATHO)$(TESTPREFIX)*.o
	$(CLEANUP) $(PATHB)*$(TARGET_EXTENSION)

.PRECIOUS: $(PATHB)$(TESTPREFIX)%$(TARGET_EXTENSION)
.PRECIOUS: $(PATHD)%.d
.PRECIOUS: $(PATHO)%.o
.PRECIOUS: $(PATHO)$(TESTPREFIX)$(TARGETPREFIX)%.o
