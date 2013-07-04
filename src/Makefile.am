pkglibexec_PROGRAMS = indicator-sound-service

indicator_sound_service_SOURCES = \
	service.vala \
	main.vala \
	volume-control.vala \
	media-player.vala \
	media-player-list.vala \
	mpris2-interfaces.vala \
	mpris2-watcher.vala \
	freedesktop-interfaces.vala

indicator_sound_service_VALAFLAGS = \
  --ccode \
  --vapidir=$(top_srcdir)/vapi/ \
  --vapidir=./ \
  --thread \
  --pkg config \
  --pkg gio-2.0 \
  --pkg gio-unix-2.0 \
  --pkg libxml-2.0 \
  --pkg libpulse \
  --pkg libpulse-mainloop-glib \
  --target-glib=2.36

# -w to disable warnings for vala-generated code
indicator_sound_service_CFLAGS = $(PULSEAUDIO_CFLAGS) \
	$(SOUNDSERVICE_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-DLIBEXECDIR=\"$(libexecdir)\" \
	-w \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\"

indicator_sound_service_LDADD = $(PULSEAUDIO_LIBS) $(SOUNDSERVICE_LIBS) $(GCONF_LIBS)
indicator_sound_service_LDFLAGS = $(COVERAGE_LDFLAGS)
