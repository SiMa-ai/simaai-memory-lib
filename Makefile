# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2021 Sima ai
#

CC       = $(CROSS_COMPILE)gcc
STRIP    = $(CROSS_COMPILE)strip
DEPFLAGS = -MD
CFLAGS   = -Wall -O2

# Library
LIBNAME   = simaaimem
LIBSONAME = $(addprefix lib, $(addsuffix .so, $(LIBNAME)))
LIBSRCS   = simaai_memory.c
LIBHDRS   = simaai_memory.h
LIBOBJS  := $(addsuffix .o, $(basename $(LIBSRCS)))
LIBDEPS  := $(addsuffix .d, $(basename $(LIBSRCS)))

# Test application
APPNAME  = simaai_mem_test
APPSRCS  = main.c
APPOBJS := $(addsuffix .o, $(basename $(APPSRCS)))
APPDEPS := $(addsuffix .d, $(basename $(APPSRCS)))

.PHONY: all strip clean distclean
all: $(LIBSONAME) $(APPNAME)

$(LIBSONAME) : $(LIBOBJS)
	$(CC) $(CFLAGS) -shared -fPIC $^ -o $@ $(LDFLAGS)

$(LIBOBJS) : Makefile $(LIBHDRS)

%.o : %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) $(CPPFLAGS) -c -fPIC $< -o $@

$(APPNAME) : $(APPOBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) -L. -l$(LIBNAME)

$(APPOBJS) : $(LIBSONAME)

strip : $(APPNAME) $(LIBSONAME)
	$(STRIP) $^

distclean : clean

clean :
	rm -f $(LIBOBJS) $(LIBDEPS) $(LIBSONAME) $(APPOBJS) $(APPDEPS) $(APPNAME)

ifneq (,$(wildcard $(LIBDEPS)))
-include $(LIBDEPS)
endif

ifneq (,$(wildcard $(APPDEPS)))
-include $(APPDEPS)
endif
