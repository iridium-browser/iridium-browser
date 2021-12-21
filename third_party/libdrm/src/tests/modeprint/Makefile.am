AM_CFLAGS = \
	$(WARN_CFLAGS)\
	-fvisibility=hidden \
	-I$(top_srcdir)/include/drm \
	-I$(top_srcdir)/tests \
	-I$(top_srcdir)

if HAVE_INSTALL_TESTS
bin_PROGRAMS = \
	modeprint
else
noinst_PROGRAMS = \
	modeprint
endif

modeprint_SOURCES = \
	modeprint.c
modeprint_LDADD = \
	$(top_builddir)/tests/util/libutil.la \
	$(top_builddir)/libdrm.la
