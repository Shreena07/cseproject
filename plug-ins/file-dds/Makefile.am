## Process this file with automake to produce Makefile.in

libgimpui = $(top_builddir)/libgimp/libgimpui-$(GIMP_API_VERSION).la
libgimpconfig = $(top_builddir)/libgimpconfig/libgimpconfig-$(GIMP_API_VERSION).la
libgimpwidgets = $(top_builddir)/libgimpwidgets/libgimpwidgets-$(GIMP_API_VERSION).la
libgimp = $(top_builddir)/libgimp/libgimp-$(GIMP_API_VERSION).la
libgimpcolor = $(top_builddir)/libgimpcolor/libgimpcolor-$(GIMP_API_VERSION).la
libgimpbase = $(top_builddir)/libgimpbase/libgimpbase-$(GIMP_API_VERSION).la
libgimpmath = $(top_builddir)/libgimpmath/libgimpmath-$(GIMP_API_VERSION).la

if OS_WIN32
mwindows = -mwindows
else
libm     = -lm
endif

if HAVE_WINDRES
include $(top_srcdir)/build/windows/gimprc-plug-ins.rule
file_dds_RC = file-dds.rc.o
endif

AM_LDFLAGS = $(mwindows)
AM_CFLAGS = -fno-strict-aliasing -fopenmp

libexecdir = $(gimpplugindir)/plug-ins/file-dds

libexec_PROGRAMS = file-dds

file_dds_SOURCES = \
	dds.c		\
	dds.h		\
	color.c		\
	color.h		\
	ddsread.c	\
	ddsread.h	\
	ddswrite.c	\
	ddswrite.h	\
	dxt.c		\
	dxt.h		\
	dxt_tables.h	\
	endian_rw.h	\
	imath.h		\
	mipmap.c	\
	mipmap.h	\
	misc.c		\
	misc.h		\
	vec.h

AM_CPPFLAGS = \
	-I$(top_srcdir)	\
	$(GTK_CFLAGS)	\
	$(GEGL_CFLAGS)	\
	-I$(includedir)

LDADD = \
	$(libgimpui)		\
	$(libgimpwidgets)	\
	$(libgimpconfig)	\
	$(libgimp)		\
	$(libgimpcolor)		\
	$(libgimpmath)		\
	$(libgimpbase)		\
	$(libm)			\
	$(GTK_LIBS)		\
	$(GEGL_LIBS)		\
	$(RT_LIBS)		\
	$(INTLLIBS)		\
	$(file_dds_RC)
