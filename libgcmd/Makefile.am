## Process this file with automake to produce Makefile.in

AM_CXXFLAGS = $(WARN_CXXFLAGS)
AM_LDFLAGS = $(WARN_LDFLAGS)

AM_CPPFLAGS = \
	$(GNOMEUI_CFLAGS) \
	$(GNOMEVFS_CFLAGS) \
	-DGTK_DISABLE_SINGLE_INCLUDES \
	-DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES

pkglib_LTLIBRARIES = libgcmd.la

libgcmd_la_SOURCES = \
	gnome-cmd-plugin.h gnome-cmd-plugin.cc \
	gnome-cmd-file-info.h gnome-cmd-file-info.cc \
	gnome-cmd-dialog.h gnome-cmd-dialog.cc \
	gnome-cmd-string-dialog.h gnome-cmd-string-dialog.cc \
	gnome-cmd-state.h \
	libgcmd-utils.h libgcmd-utils.cc \
	libgcmd-widget-factory.h libgcmd-widget-factory.cc \
	plugin-info.h \
	libgcmd-deps.h \
	libgcmd.h

libgcmd_la_LIBADD = $(GNOMEUI_LIBS) $(GNOMEVFS_LIBS)
libgcmd_la_LDFLAGS = $(GNOMEUI_LIBS) $(GNOMEVFS_LIBS)

-include $(top_srcdir)/git.mk
