libexec_PROGRAMS = indicator-sound-service

#####################
# Sound service vala
#####################
music_bridge_VALASOURCES = \
	service.vala \
	main.vala \
	volume-control.vala \
	media-player.vala \
	media-player-list.vala \
	mpris2-interfaces.vala \
	mpris2-watcher.vala \
	freedesktop-interfaces.vala

music_bridge_VALAFLAGS = \
  --ccode \
  -H music-player-bridge.h -d . \
  --vapidir=$(top_srcdir)/vapi/ \
  --vapidir=./ \
  --thread \
  --pkg gee-1.0 \
  --pkg Dbusmenu-0.4 \
  --pkg common-defs \
  --pkg config \
  --pkg gio-2.0 \
  --pkg gio-unix-2.0 \
  --pkg gdk-pixbuf-2.0 \
  --pkg libxml-2.0 \
  --pkg libpulse \
  --pkg libpulse-mainloop-glib \
  --target-glib=2.36

 $(MAINTAINER_VALAFLAGS)

music_bridge_APIFILES = \
  music-player-bridge.h 

music_bridge_vala.stamp $(music_bridge_APIFILES): $(music_bridge_VALASOURCES)
	$(AM_V_GEN) $(VALAC) $(music_bridge_VALAFLAGS) $^
	touch music_bridge_vala.stamp

###############################
# Sound Service C
###############################
indicator_sound_service_SOURCES = \
  common-defs.h \
  $(music_bridge_VALASOURCES:.vala=.c)

indicator_sound_service_CFLAGS = $(PULSEAUDIO_CFLAGS) \
	$(SOUNDSERVICE_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-DLIBEXECDIR=\"$(libexecdir)\" -Wall
indicator_sound_service_LDADD = $(PULSEAUDIO_LIBS) $(SOUNDSERVICE_LIBS) $(GCONF_LIBS)
indicator_sound_service_LDFLAGS = $(COVERAGE_LDFLAGS)

EXTRA_DIST = \
  $(DBUS_SPECS) \
  $(music_bridge_APIFILES) \
  $(music_bridge_VALASOURCES)
  
#######################
# Stuff to clean Stuff
#######################
BUILT_SOURCES = \
	music_bridge_vala.stamp \
	$(music_bridge_APIFILES) \
	$(music_bridge_VALASOURCES:.vala=.c)
  
CLEANFILES = \
	$(BUILT_SOURCES)

DISTCLEANFILES = 
