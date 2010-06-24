libexec_PROGRAMS = indicator-sound-service

###################
# Indicator Stuff
###################

soundmenulibdir = $(INDICATORDIR)
soundmenulib_LTLIBRARIES = libsoundmenu.la
libsoundmenu_la_SOURCES = \
        common-defs.h \
	indicator-sound.h \
	transport-widget.c \
	transport-widget.h \
	metadata-widget.c \
	metadata-widget.h \
	indicator-sound.c \
	dbus-shared-names.h \
  sound-service-client.h 

libsoundmenu_la_CFLAGS = $(APPLET_CFLAGS) -Wall -Werror -DG_LOG_DOMAIN=\"Indicator-Sound\"
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


####################################################################
# Sound Service 
####################################################################

#################
# dbus interface
#################
sound-service-server.h: $(srcdir)/sound-service.xml
	dbus-binding-tool \
		--prefix=_sound_service_server \
		--mode=glib-server \
		--output=sound-service-server.h \
		$(srcdir)/sound-service.xml

#####################
# libsoundmenu vala
#####################
music_bridge_VALASOURCES = \
	music-player-bridge.vala \
	transport-menu-item.vala \
	metadata-menu-item.vala \
	player-controller.vala \
	mpris-controller-v2.vala \
	mpris-controller.vala \
	player-item.vala \
	familiar-players-db.vala

music_bridge_VALAFLAGS = \
  --ccode \
  -H music-player-bridge.h -d . \
  --library music-bridge \
  --vapidir=$(top_srcdir)/vapi/ \
  --vapidir=./ \
  --thread \
  --pkg gee-1.0 \
  --pkg Indicate-0.2 \
  --pkg Dbusmenu-Glib-0.2 \
  --pkg common-defs \
	--pkg dbus-glib-1 \
	--pkg gio-unix-2.0
 
  $(MAINTAINER_VALAFLAGS)

music_bridge_APIFILES = \
  music-player-bridge.h 

music_bridge_vala.stamp $(music_bridge_APIFILES): $(music_bridge_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(music_bridge_VALAFLAGS) $^
	touch music_bridge_vala.stamp

###############################
# libsoundmenu C
###############################
indicator_sound_service_SOURCES = \
	common-defs.h \
  sound-service.h \
  sound-service.c \
  dbus-menu-manager.c \
  dbus-menu-manager.h \
  pulse-manager.h \
  pulse-manager.c \
  sound-service-dbus.h \
  sound-service-dbus.c \
  sound-service-server.h \
  slider-menu-item.h \
  slider-menu-item.c \
  $(music_bridge_VALASOURCES:.vala=.c)

indicator_sound_service_CFLAGS = $(PULSEAUDIO_CFLAGS) $(SOUNDSERVICE_CFLAGS) $(GCONF_CFLAGS) -DLIBEXECDIR=\"$(libexecdir)\" -Wall
indicator_sound_service_LDADD = $(PULSEAUDIO_LIBS) $(SOUNDSERVICE_LIBS) $(GCONF_LIBS)

#########################
# Extra stuff to package
#########################
EXTRA_DIST = \
  sound-service.xml \
  sound-service.list \
  $(music_bridge_APIFILES) \
  $(music_bridge_VALASOURCES)

#######################
# Stuff to clean Stuff
#######################
BUILT_SOURCES = \
	music_bridge_vala.stamp \
	sound-service-client.h \
  sound-service-server.h \
  music-bridge.vapi \
  $(music_bridge_APIFILES) \
  $(music_bridge_VALASOURCES:.vala=.c)

CLEANFILES = \
	$(BUILT_SOURCES)

