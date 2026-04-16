# SPDX-License-Identifier: (GPL-2.0+ OR MIT)
#
# Copyright (c) 2021 Sima ai
#

CC       = $(CROSS_COMPILE)gcc
STRIP    = $(CROSS_COMPILE)strip
DEPFLAGS = -MD
CFLAGS   = -Wall -O2 -I .

# Library
SONAME_MAJOR := 2
VERSION      := 2.1.0
LIBNAME   = simaaimem
LIBSONAME = $(addprefix lib, $(addsuffix .so, $(LIBNAME)))
LIBSRCS   = simaai_memory.c
LIBHDRS   = simaai_memory.h
LIBOBJS  := $(addsuffix .o, $(basename $(LIBSRCS)))
LIBDEPS  := $(addsuffix .d, $(basename $(LIBSRCS)))
REAL_LIB := $(LIBSONAME).$(VERSION)
SONAME   := $(LIBSONAME).$(SONAME_MAJOR)

# Test application
APPNAME  = simaai_mem_test
APPSRCS  = main.c
APPOBJS := $(addsuffix .o, $(basename $(APPSRCS)))
APPDEPS := $(addsuffix .d, $(basename $(APPSRCS)))

PREFIX ?= /usr
LIBDIR ?= $(PREFIX)/lib
INCDIR ?= $(PREFIX)/include/simaai
BINDIR ?= $(PREFIX)/bin

#Package details
PKGNAME ?= simaai-memory-lib
PKGCONFIGDIR ?= $(LIBDIR)/pkgconfig
PKGCFGFILE ?= $(PKGNAME).pc
CMAKEDIR ?= $(LIBDIR)/cmake/$(PKGNAME)

.PHONY: all strip install clean distclean
all: $(REAL_LIB) $(APPNAME) $(PKGCFGFILE)

$(REAL_LIB) : $(LIBOBJS)
	$(CC) $(CFLAGS) -shared -fPIC -Wl,-soname,$(SONAME) $^ -o $@ $(LDFLAGS)
	ln -sf $@ $(LIBSONAME)

$(LIBOBJS) : Makefile $(LIBHDRS)

%.o : %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) $(CPPFLAGS) -c -fPIC $< -o $@

$(APPNAME) : $(APPOBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) -L. -l$(LIBNAME)

$(APPOBJS) : $(REAL_LIB)

strip : $(APPNAME) $(REAL_LIB)
	$(STRIP) $^

$(PKGCFGFILE): $(PKGCFGFILE).in
	sed -e "s|@VERSION@|$(VERSION)|g" $< > $@

install: $(REAL_LIB)
	install -d $(DESTDIR)$(LIBDIR)
	install -m 0755 $(REAL_LIB) $(DESTDIR)$(LIBDIR)
	ln -sf $(REAL_LIB) $(DESTDIR)$(LIBDIR)/$(SONAME)
	ln -sf $(SONAME)  $(DESTDIR)$(LIBDIR)/$(LIBSONAME)

	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(APPNAME) $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(INCDIR)
	install -m 0755 $(LIBHDRS) $(DESTDIR)$(INCDIR)

	install -d $(DESTDIR)$(PKGCONFIGDIR)
	install -m 0644 $(PKGCFGFILE) $(DESTDIR)$(PKGCONFIGDIR)/$(PKGCFGFILE)
	install -d $(DESTDIR)$(CMAKEDIR)
	install -m 0644 cmake/$(PKGNAME)Targets.cmake $(DESTDIR)$(CMAKEDIR)
	install -m 0644 cmake/$(PKGNAME)Config.cmake $(DESTDIR)$(CMAKEDIR)

distclean : clean

clean :
	rm -f $(LIBOBJS) $(LIBDEPS) $(LIBSONAME).* $(APPOBJS) $(APPDEPS) $(APPNAME)

ifneq (,$(wildcard $(LIBDEPS)))
-include $(LIBDEPS)
endif

ifneq (,$(wildcard $(APPDEPS)))
-include $(APPDEPS)
endif
