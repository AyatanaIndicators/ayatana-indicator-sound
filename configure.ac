
AC_INIT(indicator-sound, 0.8.5.0, conor.curran@canonical.com)

AC_PREREQ(2.53)

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(indicator-sound, 0.8.5.0)

AM_MAINTAINER_MODE

IT_PROG_INTLTOOL([0.35.0])
AM_PROG_VALAC([0.11.2])
AS_IF([test -z "$VALAC"], [AC_MSG_ERROR(["No valac compiler found."])])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS
AC_PROG_LIBTOOL

AC_SUBST(VERSION)
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PATH_PROG([GLIB_GENMARSHAL], [glib-genmarshal])

###########################
# GTK+ version option
###########################

AC_ARG_WITH([gtk],
  [AS_HELP_STRING([--with-gtk],
    [Which version of gtk to use for the indicator @<:@default=3@:>@])],
  [],
  [with_gtk=3])

###########################
# Dependencies 
###########################  

GTK_REQUIRED_VERSION=2.22
GTK3_REQUIRED_VERSION=3.0
INDICATOR_REQUIRED_VERSION=0.3.19
DBUSMENUGTK_REQUIRED_VERSION=0.5.90
POLKIT_REQUIRED_VERSION=0.92
PULSE_AUDIO_REQUIRED_VERSION=0.9.19
INDICATOR_DISPLAY_OBJECTS=0.1.11
DBUSMENUGLIB_REQUIRED_VERSION=0.3.101
GIO_2_0_REQUIRED_VERSION=2.25.13
LIBNOTIFY_REQUIRED_VERSION=0.7.0

PKG_CHECK_MODULES(PULSEAUDIO, libpulse-mainloop-glib >= $PULSE_AUDIO_REQUIRED_VERSION
                              gio-unix-2.0)
AC_SUBST(PULSEAUDIO_CFLAGS)
AC_SUBST(PULSEAUDIO_LIBS)

AS_IF([test "x$with_gtk" = x3],
    [PKG_CHECK_MODULES(APPLET, gtk+-3.0 >= $GTK3_REQUIRED_VERSION
                               indicator3-0.4 >= $INDICATOR_REQUIRED_VERSION
                               dbusmenu-gtk3-0.4 >= $DBUSMENUGTK_REQUIRED_VERSION
                               libido3-0.1 >= $INDICATOR_DISPLAY_OBJECTS
                               libnotify >= $LIBNOTIFY_REQUIRED_VERSION)

     PKG_CHECK_MODULES(SOUNDSERVICE, dbusmenu-glib-0.4 >= $DBUSMENUGLIB_REQUIRED_VERSION
                                     indicator3-0.4
                                     gee-1.0
                                     gio-unix-2.0
                                     libxml-2.0)
        ],
      [test "x$with_gtk" = x2],
    [PKG_CHECK_MODULES(APPLET, gtk+-2.0 >= $GTK_REQUIRED_VERSION
                               indicator-0.4 >= $INDICATOR_REQUIRED_VERSION
                               dbusmenu-gtk-0.4 >= $DBUSMENUGTK_REQUIRED_VERSION
                               libido-0.1 >= $INDICATOR_DISPLAY_OBJECTS
                               libnotify >= $LIBNOTIFY_REQUIRED_VERSION)

     PKG_CHECK_MODULES(SOUNDSERVICE, dbusmenu-glib-0.4 >= $DBUSMENUGLIB_REQUIRED_VERSION
                                     indicator-0.4
                                     gee-1.0
                                     gio-unix-2.0
                                     libxml-2.0)
        ],
    [AC_MSG_FAILURE([Value for --with-gtk was neither 2 nor 3])]
)
AC_SUBST(APPLET_CFLAGS)
AC_SUBST(APPLET_LIBS)

AC_SUBST(SOUNDSERVICE_CFLAGS)
AC_SUBST(SOUNDSERVICE_LIBS)

###########################
# gcov coverage reporting
###########################

m4_include([m4/gcov.m4])
AC_TDD_GCOV
AM_CONDITIONAL([HAVE_GCOV], [test "x$ac_cv_check_gcov" = xyes])
AM_CONDITIONAL([HAVE_LCOV], [test "x$ac_cv_check_lcov" = xyes])
AM_CONDITIONAL([HAVE_GCOVR], [test "x$ac_cv_check_gcovr" = xyes])
AC_SUBST(COVERAGE_CFLAGS)
AC_SUBST(COVERAGE_CXXFLAGS)
AC_SUBST(COVERAGE_LDFLAGS)

###########################
# Check to see if we're local
###########################

with_localinstall="no"
AC_ARG_ENABLE(localinstall, AS_HELP_STRING([--enable-localinstall], [install all of the files localy instead of system directories (for distcheck)]), with_localinstall=$enableval, with_localinstall=no)

###########################
# Indicator Info
###########################

AS_IF([test "x$with_localinstall" = "xyes"],
	[
	INDICATORDIR="${libdir}/indicators/2/"
	INDICATORICONSDIR="${datadir}/indicator-sound/icons/"
	],
	[AS_IF([test "x$with_gtk" = "x2"],
		[
		INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator-0.4`
		INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator-0.4`
		],
		[
		INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator3-0.4`
		INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator3-0.4`
		])])
AC_SUBST(INDICATORDIR)
AC_SUBST(INDICATORICONSDIR)

###########################
# Grab the GSettings Macros
###########################

GLIB_GSETTINGS
GLIB_GENMARSHAL

###########################
# DBus Service Info
###########################

if test "x$with_localinstall" = "xyes"; then
	DBUSSERVICEDIR="${datadir}/dbus-1/services/"
else
	DBUSSERVICEDIR=`$PKG_CONFIG --variable=session_bus_services_dir dbus-1`
fi
AC_SUBST(DBUSSERVICEDIR)

##############################
# Custom Junk
##############################

AC_DEFUN([AC_DEFINE_PATH], [
	test "x$prefix" = xNONE && prefix="$ac_default_prefix"
	test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
	ac_define_path=`eval echo [$]$2`
	ac_define_path=`eval echo [$]ac_define_path`
	$1="$ac_define_path"
	AC_SUBST($1)
	ifelse($3, ,
		AC_DEFINE_UNQUOTED($1, "$ac_define_path"),
		AC_DEFINE_UNQUOTED($1, "$ac_define_path", $3))
])

###########################
# Internationalization
###########################

GETTEXT_PACKAGE=indicator-sound
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Name of the default get text domain])
AC_DEFINE_PATH(GNOMELOCALEDIR, "${datadir}/locale", [locale directory])

AM_GLIB_GNU_GETTEXT

###########################
# Files
###########################

AC_OUTPUT([
Makefile
src/Makefile
data/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/16x16/status/Makefile
data/icons/scalable/Makefile
data/icons/scalable/status/Makefile
po/Makefile.in
])

###########################
# Results
###########################

AC_MSG_NOTICE([

SUS Indicator Configuration:

	Prefix:        $prefix
	GTK+:          $with_gtk
	gcov:          $use_gcov
])
