
check_PROGRAMS = \
	test-indicator-sound

TESTS =
DISTCLEANFILES = $(TESTS)

#########################################
##  test-indicator-sound
#########################################
test_indicator_sound_SOURCES = \
	test-indicator-sound.c
    
test_indicator_sound_CFLAGS = \
	$(APPLET_CFLAGS) \
	-Wall -Werror \
	-I$(srcdir) \
	-DTOP_BUILD_DIR="\"${abs_top_builddir}\""

test_indicator_sound_LDADD = \
	$(APPLET_LIBS) \
	$(INDICATORDIR)/libsoundmenu.la

#DBUS_RUNNER=dbus-test-runner --dbus-config /usr/share/dbus-test-runner/session.conf

#test-indicator-sound-dbus: test-indicator-sound-dbus-client test-indicator-sound-dbus-server Makefile.am
#	@echo "#!/bin/sh" > test-indicator-sound-dbus
#	@echo $(DBUS_RUNNER) --task ./test-indicator-sound-dbus-client --task-name Client --task ./test-indicator-sound-dbus-server --task-name Server --ignore-return >> test-indicator-sound-dbus
#	@chmod +x test-libappindicator-dbus

#TESTS += test-libappindicator-dbus

XVFB_RUN=". $(srcdir)/run-xvfb.sh"

EXTRA_DIST = run-xvfb.sh


#############################
# Test stuff
#############################

#XML_REPORT  = loader-check-results.xml
#HTML_REPORT = loader-check-results.html

#loader-tester: test-loader libsoundmenu.la Makefile
#	@echo "#!/bin/bash" > loader-tester
#	@echo $(XVFB_RUN) >> $@
#	@echo gtester -k --verbose -o=$(XML_REPORT) ./test-loader >> loader-tester
#	@chmod +x loader-tester

#TESTS += loader-tester
#DISTCLEANFILES += loader-tester

#DISTCLEANFILES += $(XML_REPORT) $(HTML_REPORT)

