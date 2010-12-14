SUBDIRS = icons

dbus_servicesdir = $(DBUSSERVICEDIR)
service_in_files =  indicator-sound.service.in 
dbus_services_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
  sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

##############################
# GSettings Schema
##############################

gsettings_SCHEMAS = \
  com.canonical.indicators.sound.gschema.xml
@GSETTINGS_RULES@

##############################

EXTRA_DIST =          \
  $(service_in_files)     \
  $(gsettings_SCHEMAS) 

CLEANFILES =          \
  $(dbus_services_DATA)
  