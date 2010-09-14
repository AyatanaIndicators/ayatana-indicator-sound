SUBDIRS = icons

dbus_servicesdir = $(DBUSSERVICEDIR)
service_in_files =  indicator-sound.service.in 
dbus_services_DATA = $(service_in_files:.service.in=.service)

%.service: %.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

##############################
# GConf Schema
##############################

schemadir =					\
	$(GCONF_SCHEMA_FILE_DIR)

schema_in_files =				\
	indicator-sound.schemas.in

schema_DATA =					\
	$(schema_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@


#$(dbus_services_DATA): $(service_in_files) Makefile
#	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST =					\
	$(service_in_files)			\
	$(schema_in_files)

CLEANFILES =					\
	$(dbus_services_DATA)			\
	$(schema_DATA)

if GCONF_SCHEMAS_INSTALL
install-data-local:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) \
		gconftool-2 --makefile-install-rule $(schema_DATA)
else
install-data-local:
endif
