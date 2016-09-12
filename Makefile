#
# Copyright (C) 2010 by Chris Evich <cevich@redhat.com>
#
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Lesser General Public
#   License as published by the Free Software Foundation; either
#   version 2.1 of the License, or (at your option) any later version.
#
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Lesser General Public License for more details.
#
#   You should have received a copy of the GNU Lesser General Public
#   License along with this library; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
#
#Use: This Makefile is designed to be as automatic as possible
#     Create *.deps files for anything that needs to be linked
#     with non-obvious other things. These *.deps files
#     are simply recursivly included into this makefile so their format is
#     the simple "name1: name2.o name3.o".  Specify the default names below,
#     of stuff to build when doing a make all.
#Note: If editing with vi, don't forget to "set noet" and "set nosta".

NAMES=ringwrap #Names of the stuff to build for a 'make all'
CPPFLAGS= #Preprocessing flags to use
LOADLIBES= #Static loadable libraries to link in.
LDLIBS= -lrt # shared libraries to link in.
CC=gcc #Use the GNU C Compiler
# Default flags to use when compiling.
CFLAGS=-D__USE_FIXED_PROTOTYPES__ -Wall -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE -g 
#Default flags to use when linking.
LDFLAGS=-shared-libgcc
MAINTAINERFLAGS=-DMAINTAINER #Define used to enable maintainer mode
SOURCES=$(strip $(shell find . -name "*.c"))
DEPS=$(strip $(shell find . -name "*.deps"))
#Default target, depends on all automatic dependencies and other stuff
ALL: $(LOADLIBES) $(NAMES)

#Bring in any explicit dependency exceptions
ifneq ($(DEPS),)
-include $(DEPS)
endif

#Rule to automatically build target dependency files
%.d: %.c
	@echo "Building dependencies for $@"
	@set -e; rm -f $@;\
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$;\
	sed 's,\($(*F)\.o\)[ :]*,$*.o $*.d : ,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

# Bring in all auto-generated dependency files
-include $(SOURCES:.c=.d)

%.so: %.o
	$(CC) -shared $(LDFLAGS) -Wl,-soname,$(*F).so -o $@ $^ 

.PHONY : clean maintainer

maintainer:
	@echo "";\
	echo "";\
	echo "WARNING: BUILDING IN MAINTAINER DEBUG MODE.";\
	echo "";\
	echo "";\
	$(MAKE) "CPPFLAGS=$(MAINTAINERFLAGS)" "CFLAGS=$(CFLAGS) -g"

clean:
	#Remove all of the main names, object, shared object, and dependency files.
	-for name in $(NAMES) $(CLEANME)\
		$(shell find . -name "*.o")\
		$(shell find . -name "*.d.*")\
		$(shell find . -name "*.so")\
		$(shell find . -name "*.pic_o")\
		$(shell find . -name "*.d");\
			do rm -f $$name;\
	done
