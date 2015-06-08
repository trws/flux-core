AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = $(shell python-config --cflags )\
	$(JSON_CFLAGS) \
	-I$(top_srcdir) -I$(top_srcdir)/src/include

#
# Comms module
#
fluxmod_LTLIBRARIES = pymod.la

fluxmod_libadd = \
	$(top_builddir)/src/common/libflux-core.la \
	$(JSON_LIBS) $(LIBZMQ) $(LIBCZMQ)

fluxmod_ldflags = -avoid-version -module -shared -export-dynamic \
	-export-symbols-regex '^mod_(main|name)$$'

pymod_la_SOURCES = py_mod.c
pymod_la_LDFLAGS = $(fluxmod_ldflags) "-Wl,-rpath,$(shell python-config --exec-prefix)/lib" "-L$(shell python-config --exec-prefix)/lib" $(shell python-config --ldflags)
pymod_la_LIBADD = $(fluxmod_libadd)

#
# API for module
#
# fluxcoreinclude_HEADERS = barrier.h
# noinst_LTLIBRARIES = libbarrier.la
# libbarrier_la_SOURCES = libbarrier.c
