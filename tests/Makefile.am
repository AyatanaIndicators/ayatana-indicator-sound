
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
	-DBUILD_DIR="\"$(builddir)\""

test_indicator_sound_LDADD = \
	$(APPLET_LIBS) \
	$(INDICATORDIR)/libsoundmenu.la

