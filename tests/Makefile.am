check_PROGRAMS = \
	test-indicator-sound \ 
	test-pulse-manager \
	test-indicator-sound-dbus-client \
	test-indicator-sound-dbus-server 

TESTS = \
	test-indicator-sound \
	test-pulse-manager

DISTCLEANFILES = $(TESTS)

noinst_LTLIBRARIES        = libmockpulse.la
libmockpulse_la_SOURCES   = mockpulse.c mockpulse.h
libmockpulse_la_CFLAGS    = $(PULSEAUDIO_CFLAGS)


#########################################
##  test-indicator-sound
#########################################
test_indicator_sound_SOURCES = \
	test-indicator-sound.c \
    $(top_builddir)/src/indicator-sound.c
    
test_indicator_sound_CFLAGS = \
	$(PULSEAUDIO_CFLAGS) \
	$(APPLET_CFLAGS) \
	-Wall -Werror \
	-I$(srcdir) \
	-DTOP_BUILD_DIR="\"${abs_top_builddir}\""

test_indicator_sound_LDADD = \
	libmockpulse.la \
	$(PULSEAUDIO_LIBS) \
	$(APPLET_LIBS) 


#########################################
##  test-pulse-manager
#########################################

check_PROGRAMS += test-pulse-manager

test_pulse_manager_SOURCES = \
	test-pulse-manager.c \
    $(top_builddir)/src/dbus-menu-manager.c \
    $(top_builddir)/src/sound-service-dbus.c \ 
    $(top_builddir)/src/slider-menu-item.c 

test_pulse_manager_CFLAGS = \
	$(PULSEAUDIO_CFLAGS) \
	$(SOUNDSERVICE_CFLAGS) \
	$(GCONF_CFLAGS) \
	-Wall -Werror \
	-I$(srcdir) \
	-I$(SOUNDSERVICE_CFLAGS)

test_pulse_manager_LDADD = \
	libmockpulse.la \
	$(PULSEAUDIO_LIBS) \
	$(SOUNDSERVICE_LIBS) \
	$(GCONF_LIBS) \
	$(APPLET_LIBS) 

#########################################
##  test-indicator-sound-dbus-client
#########################################
test_indicator_sound_dbus_client_SOURCES = \
	test-defines.h \
	test-indicator-sound-dbus-client.c

test_indicator_sound_dbus_client_CFLAGS = \
	$(PULSEAUDIO_CFLAGS) \
	$(SOUNDSERVICE_CFLAGS) \
	-Wall -Werror \
	-I$(srcdir) \
	-I$(SOUNDSERVICE_CFLAGS)

test_indicator_sound_dbus_client_LDADD = \
	$(PULSEAUDIO_LIBS) \
	$(SOUNDSERVICE_LIBS)

########################################
#  test-indicator-sound-dbus-server
########################################
test_indicator_sound_dbus_server_SOURCES = \
	test-defines.h \
    test-indicator-sound-dbus-server.c \
    $(top_builddir)/src/sound-service-dbus.c \
    $(top_builddir)/src/pulse-manager.c \
    $(top_builddir)/src/slider-menu-item.c \
    $(top_builddir)/src/dbus-menu-manager.c

test_indicator_sound_dbus_server_CFLAGS = \
    $(SOUNDSERVICE_CFLAGS) \
	-Wall -Werror \
	-I$(srcdir) 

test_indicator_sound_dbus_server_LDADD = \
	$(SOUNDSERVICE_LIBS) \
	$(PULSEAUDIO_LIBS) 


#########################################
##  Actual tests
#########################################

XML_REPORT   = indicator-sound-check-results.xml
HTML_REPORT  = indicator-sound-check-results.html

indicator-sound-tests: indicator-sound-tests-gtester Makefile.am
	@echo "#!/bin/sh" > $@
	@echo $(DBUS_RUNNER) --task ./indicator-sound-tests-gtester >> $@
	@chmod +x $@

indicator-sound-tests-gtester: test-indicator-sound Makefile.am
	@echo "#!/bin/sh" > $@
	@echo gtester -k --verbose -o=indicator-sound-check-results.xml ./test-indicator-sound >> $@
	@chmod +x $@

TESTS += indicator-sound-tests-gtester

pulse-manager-tests-gtester: test-pulse-manager Makefile.am
	@echo "#!/bin/sh" > $@
	@echo gtester -k --verbose -o=pulse-manager-check-results.xml ./test-pulse-manager >> $@
	@chmod +x $@

TESTS += pulse-manager-tests-gtester

DISTCLEANFILES += $(XML_REPORT) $(HTML_REPORT) indicator-sound-tests-gtester

DBUS_RUNNER=dbus-test-runner --dbus-config /usr/share/dbus-test-runner/session.conf

test-indicator-sound-dbus: test-indicator-sound-dbus-client test-indicator-sound-dbus-server Makefile.am
	@echo "#!/bin/sh" > test-indicator-sound-dbus
	@echo $(DBUS_RUNNER) --task ./test-indicator-sound-dbus-server --task-name Server --ignore-return >> test-indicator-sound-dbus --task ./test-indicator-sound-dbus-client --task-name Client
	@chmod +x test-indicator-sound-dbus

TESTS += test-indicator-sound-dbus






