## Process this file with automake to produce Makefile.in

noinst_LIBRARIES = libgcmd-tags.a

AM_CXXFLAGS = $(WARN_CXXFLAGS)
AM_LDFLAGS = $(WARN_LDFLAGS)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(srcdir)/.. \
	$(GNOMEUI_CFLAGS) \
	$(GNOMEVFS_CFLAGS) \
	$(EXIV2_CFLAGS) \
	$(TAGLIB_CFLAGS) \
	$(CHM_CFLAGS) \
	$(GSF_CFLAGS) \
	$(POPPLER_CFLAGS) \
	-DGTK_DISABLE_SINGLE_INCLUDES \
	-DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES \
	-DPLUGIN_DIR=\""$(libdir)/$(PACKAGE)/plugins"\"

libgcmd_tags_a_SOURCES = \
	gnome-cmd-tags.h gnome-cmd-tags.cc \
	gnome-cmd-tags-doc.h gnome-cmd-tags-doc.cc \
	gnome-cmd-tags-exiv2.h gnome-cmd-tags-exiv2.cc \
	gnome-cmd-tags-file.h gnome-cmd-tags-file.cc \
	gnome-cmd-tags-poppler.h gnome-cmd-tags-poppler.cc \
	gnome-cmd-tags-taglib.h gnome-cmd-tags-taglib.cc

-include $(top_srcdir)/git.mk
