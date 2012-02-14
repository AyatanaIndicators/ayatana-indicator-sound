SUBDIRS = icons

gsettings_SCHEMAS = \
  com.canonical.indicator.sound.gschema.xml
@GSETTINGS_RULES@

dbus_servicesdir = $(DBUSSERVICEDIR)
dbus_services_DATA = indicator-sound.service

%.service: %.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = \
  $(gsettings_SCHEMAS) \
  indicator-sound.service.in

CLEANFILES = \
  $(dbus_services_DATA)
  