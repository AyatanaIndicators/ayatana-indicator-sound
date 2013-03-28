libexec_PROGRAMS = indicator-sound-service

checkxml: $(srcdir)/sound-service.xml
	@xmllint -valid -noout $<
	@echo $< checks out ok


####################################################################
# Sound Service 
####################################################################

glib_marshal_list = sound-service-marshal.list
glib_marshal_prefix = _sound_service_marshal


#####################
# Sound service vala
#####################
music_bridge_VALASOURCES = \
	service.vala \
	main.vala \
	volume-control.vala \
	media-player.vala \
	media-player-list.vala \
	music-player-bridge.vala \
	transport-menu-item.vala \
	specific-items-manager.vala \
	metadata-menu-item.vala \
	player-controller.vala \
	mpris2-interfaces.vala \
	mpris2-watcher.vala \
	mpris2-controller.vala \
	player-item.vala \
	settings-manager.vala \
	playlists-menu-item.vala \
	freedesktop-interfaces.vala \
	fetch-file.vala  

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
  --pkg pulseaudio-mgr \
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
  sound-state.c \
  sound-state.h \
  pulseaudio-mgr.h \
  pulseaudio-mgr.c \
  device.c \
  device.h \
  sound-service-dbus.h \
  sound-service-dbus.c \
  slider-menu-item.h \
  slider-menu-item.c \
  voip-input-menu-item.h \
  voip-input-menu-item.c \
  mute-menu-item.h \
  mute-menu-item.c \
  gen-sound-service.xml.h \
  gen-sound-service.xml.c \
  sound-service-marshal.c \
  sound-service-marshal.h \
  $(music_bridge_VALASOURCES:.vala=.c)

indicator_sound_service_CFLAGS = $(PULSEAUDIO_CFLAGS) \
	$(SOUNDSERVICE_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-DLIBEXECDIR=\"$(libexecdir)\" -Wall
indicator_sound_service_LDADD = $(PULSEAUDIO_LIBS) $(SOUNDSERVICE_LIBS) $(GCONF_LIBS)
indicator_sound_service_LDFLAGS = $(COVERAGE_LDFLAGS)

#########################
# Service xml compilation
#########################
DBUS_SPECS = \
  sound-service.xml

gen-%.xml.h: %.xml
	@echo "Building $@ from $<"
	@echo "extern const char * _$(subst -,_,$(subst .,_,$(basename $(notdir $<))));" > $@

gen-%.xml.c: %.xml
	@echo "Building $@ from $<"
	@echo "const char * _$(subst -,_,$(subst .,_,$(basename $(notdir $<)))) = " > $@
	@sed -e "s:\":\\\\\":g" -e s:^:\": -e s:\$$:\\\\n\": $< >> $@
	@echo ";" >> $@


EXTRA_DIST = \
  $(DBUS_SPECS) \
  sound-service.xml \
  $(music_bridge_APIFILES) \
  $(music_bridge_VALASOURCES)
  
#######################
# Stuff to clean Stuff
#######################
BUILT_SOURCES = \
	music_bridge_vala.stamp \
	$(music_bridge_APIFILES) \
	gen-sound-service.xml.h \
	gen-sound-service.xml.c \
	$(music_bridge_VALASOURCES:.vala=.c)
  
CLEANFILES = \
	$(BUILT_SOURCES)

DISTCLEANFILES = 

include $(top_srcdir)/Makefile.am.marshal
