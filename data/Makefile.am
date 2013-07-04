gsettings_SCHEMAS = \
  com.canonical.indicator.sound.gschema.xml
@GSETTINGS_RULES@

dbus_servicesdir = $(datadir)/dbus-1/services
dbus_services_DATA = indicator-sound.service

%.service: %.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

indicatordir = $(INDICATOR_DIR)
dist_indicator_DATA = com.canonical.indicator.sound

EXTRA_DIST = \
  $(gsettings_SCHEMAS) \
  indicator-sound.service.in

CLEANFILES = \
  $(dbus_services_DATA)
  
