# Copyright 2012-2016, Stephen Fryatt (info@stevefryatt.org.uk)
#
# This file is part of Locate:
#
#   http://www.stevefryatt.org.uk/software/
#
# Licensed under the EUPL, Version 1.1 only (the "Licence");
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

# This file really needs to be run by GNUMake.
# It is intended for native compilation on Linux (for use in a GCCSDK
# environment) or cross-compilation under the GCCSDK.

ARCHIVE := locate

APP := !Locate
SHHELP := Locate,3d6
HTMLHELP := manual.html

FINDSPRS := FindSprs,ffb

ADDITIONS := Extras
STARTLOCATE := StartLocate,ffb

BASDIR := bas
EXTRASRC := $(BASDIR)

PACKAGE := Locate
PACKAGELOC := File

# We need to set OUTDIR here so that EXTRAPREREQ can be set before including the master file.

OUTDIR := build
EXTRAPREREQ := $(OUTDIR)/$(APP)/$(FINDSPRS) $(OUTDIR)/$(ADDITIONS)/$(STARTLOCATE)

FINDSPRSSRC := FindSprs.bbt
STARTLOCATESRC := StartLocate.bbt

OBJS := choices.o clipboard.o contents.o datetime.o dialogue.o discfile.o	\
	file.o fileicon.o flexutils.o hotlist.o iconbar.o ignore.o main.o	\
	objdb.o	plugin.o results.o search.o settime.o textdump.o typemenu.o

include $(SFTOOLS_MAKE)/CApp

# Make FindSprs

$(OUTDIR)/$(APP)/$(FINDSPRS): $(BASDIR)/$(FINDSPRSSRC)
	@$(ECHO) "$(COLOUR_ACTION)*        TOKENIZING: $(OUTDIR)/$(APP)/$(FINDSPRS)$(COLOUR_END)"
	@$(TOKENIZE) $(TOKENIZEFLAGS) $(BASDIR)/$(FINDSPRSSRC) -out $(OUTDIR)/$(APP)/$(FINDSPRS)

# Make StartLocate

$(OUTDIR)/$(ADDITIONS)/$(STARTLOCATE): $(BASDIR)/$(STARTLOCATESRC)
	@$(ECHO) "$(COLOUR_ACTION)*        TOKENIZING: $(OUTDIR)/$(ADDITIONS)/$(STARTLOCATE)$(COLOUR_END)"
	@$(TOKENIZE) $(TOKENIZEFLAGS) $(BASDIR)/$(STARTLOCATESRC) -out $(OUTDIR)/$(ADDITIONS)/$(STARTLOCATE)

# Clean FindSprs and StartLocate

clean::
	$(RM) $(OUTDIR)/$(APP)/$(FINDSPRS)
	$(RM) $(OUTDIR)/$(ADDITIONS)/$(STARTLOCATE)

