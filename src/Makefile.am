libexec_PROGRAMS = indicator-sound-service

###################
# Indicator Stuff
###################

soundmenulibdir = $(INDICATORDIR)
soundmenulib_LTLIBRARIES = libsoundmenu.la
libsoundmenu_la_SOURCES = \
    common-defs.h \
	indicator-sound.c \
	dbus-shared-names.h \
    sound-service-client.h \
    sound-service-marshal.c \
    sound-service-marshal.h

libsoundmenu_la_CFLAGS = $(APPLET_CFLAGS) -Wall -Werror
libsoundmenu_la_LIBADD = $(APPLET_LIBS)
libsoundmenu_la_LDFLAGS = -module -avoid-version

checkxml: $(srcdir)/sound-service.xml
	@xmllint -valid -noout $<
	@echo $< checks out ok

sound-service-client.h: $(srcdir)/sound-service.xml
	dbus-binding-tool \
		--prefix=_sound_service_client \
		--mode=glib-client \
		--output=sound-service-client.h \
		$(srcdir)/sound-service.xml

sound-service-marshal.h: $(srcdir)/sound-service.list
	glib-genmarshal --header \
		--prefix=_sound_service_marshal $(srcdir)/sound-service.list \
		> sound-service-marshal.h

sound-service-marshal.c: $(srcdir)/sound-service.list
	glib-genmarshal --body \
		--prefix=_sound_service_marshal $(srcdir)/sound-service.list \
		> sound-service-marshal.c


#################
# Session Stuff
#################
indicator_sound_service_SOURCES = \
    common-defs.h \
    sound-service.h \
    sound-service.c \
    sound-service-dbus.h \
    sound-service-dbus.c \
    sound-service-server.h \
    sound-service-marshal.c \
    sound-service-marshal.h
indicator_sound_service_CFLAGS = $(SOUNDSERVICE_CFLAGS) $(GCONF_CFLAGS) -DLIBEXECDIR=\"$(libexecdir)\" -Wall -Werror
indicator_sound_service_LDADD = $(SOUNDSERVICE_LIBS) $(GCONF_LIBS)

sound-service-server.h: $(srcdir)/sound-service.xml
	dbus-binding-tool \
		--prefix=_sound_service_server \
		--mode=glib-server \
		--output=sound-service-server.h \
		$(srcdir)/sound-service.xml

###############
# Other Stuff
###############
BUILT_SOURCES = \
	sound-service-client.h \
    sound-service-server.h \
	sound-service-marshal.h \
    sound-service-marshal.c

EXTRA_DIST = \
    sound-service.xml \
    sound-service.list

CLEANFILES = \
	$(BUILT_SOURCES)





