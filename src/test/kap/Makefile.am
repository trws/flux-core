AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = \
	$(JSON_CFLAGS) $(MPI_CFLAGS) \
	-I$(top_srcdir) -I$(top_srcdir)/src/include

LDADD = \
	$(top_builddir)/src/modules/kvs/libkvs.la \
	$(top_builddir)/src/modules/api/libapi.la \
	$(top_builddir)/src/modules/barrier/libbarrier.la \
        $(top_builddir)/src/common/libflux/libflux.la \
        $(top_builddir)/src/common/libutil/libutil.la \
        $(top_builddir)/src/common/liblsd/liblsd.la \
        $(JSON_LIBS) $(LIBCZMQ) $(LIBZMQ) $(LIBMUNGE) $(LIBPTHREAD) $(LIBM) \
	$(MPI_CLDFLAGS)

check_PROGRAMS = kap

kap_SOURCES = \
	kap.h \
	kap.c \
	kap_opts.h \
	kap_opts.c \
	kap_personality.h \
	kap_personality.c \
	kap_roles.c \
	kap_timing.c

document: doc/doxy_conf.txt
	cd doc; doxygen doxy_conf.txt

TESTS = test_kap_roles

test_ldadd = \
	$(top_builddir)/src/common/libutil/libutil.la \
	$(top_builddir)/src/common/libtap/libtap.la \
	${LDADD}

test_cppflags = \
	-DTEST_MAIN \
	-I$(top_srcdir)/src/common/libtap \
	$(AM_CPPFLAGS)

check_PROGRAMS += test_kap_roles

test_kap_roles_SOURCES = kap_roles.c kap_personality.c kap_opts.c kap_timing.c
test_kap_roles_CPPFLAGS = $(test_cppflags)
test_kap_roles_LDADD = $(test_ldadd)

CLEANFILES = doc/html doc/latex doc/man
