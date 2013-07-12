gsettings_SCHEMAS = \
  com.canonical.indicator.sound.gschema.xml
@GSETTINGS_RULES@

upstart_jobsdir = $(datadir)/upstart/sessions
upstart_jobs_DATA = indicator-sound.conf

%.conf: %.conf.in
	sed -e "s|\@pkglibexecdir\@|$(pkglibexecdir)|" $< > $@

indicatordir = $(INDICATOR_DIR)
dist_indicator_DATA = com.canonical.indicator.sound

EXTRA_DIST = \
  $(gsettings_SCHEMAS) \
  indicator-sound.conf.in

CLEANFILES = \
  $(dbus_services_DATA)
  
