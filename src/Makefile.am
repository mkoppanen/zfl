lib_LTLIBRARIES = libzfl.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libzfl.pc

include_HEADERS = \
    ../include/zfl.h \
    ../include/zfl_prelude.h \
    ../include/zfl_base.h \
    ../include/zfl_config.h \
    ../include/zfl_log.h \
    ../include/zfl_json.h

libzfl_la_SOURCES = \
    zfl_base.c \
    zfl_config.c \
    zfl_log.c \
    zfl_json.c

if ON_MINGW
libzfl_la_LDFLAGS = -no-undefined -avoid-version -version-info @LTVER@ @LIBZFL_EXTRA_LDFLAGS@
else
libzfl_la_LDFLAGS = -version-info @LTVER@ @LIBZFL_EXTRA_LDFLAGS@
endif
