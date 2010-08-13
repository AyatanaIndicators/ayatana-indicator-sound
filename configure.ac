
AC_INIT(indicator-sound, 0.3.9, conor.curran@canonical.com)

AC_PREREQ(2.53)

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(indicator-sound, 0.3.9)

AM_MAINTAINER_MODE

IT_PROG_INTLTOOL([0.35.0])
AM_PROG_VALAC([0.7.8])
AS_IF([test -z "$VALAC"], [AC_MSG_ERROR(["No valac compiler found."])])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS
AC_PROG_LIBTOOL

AC_SUBST(VERSION)
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

###########################
# Dependencies 
###########################  

GTK_REQUIRED_VERSION=2.12
INDICATOR_REQUIRED_VERSION=0.3.6
DBUSMENUGTK_REQUIRED_VERSION=0.2.2
POLKIT_REQUIRED_VERSION=0.92
PULSE_AUDIO_REQUIRED_VERSION=0.9.19
INDICATOR_DISPLAY_OBJECTS=0.1.11
INDICATE_REQUIRED_VERSION=0.4.1
DBUSMENUGLIB_REQUIRED_VERSION=0.3.1
GIO_2_0_REQUIRED_VERSION=2.25.13

PKG_CHECK_MODULES(APPLET,gtk+-2.0 >= $GTK_REQUIRED_VERSION
                          indicator >= $INDICATOR_REQUIRED_VERSION
			              			dbusmenu-gtk >= $DBUSMENUGTK_REQUIRED_VERSION
                          libido-0.1 >= $INDICATOR_DISPLAY_OBJECTS)
                          
AC_SUBST(APPLET_CFLAGS)
AC_SUBST(APPLET_LIBS)


PKG_CHECK_MODULES(PULSEAUDIO, libpulse-mainloop-glib >= $PULSE_AUDIO_REQUIRED_VERSION)
AC_SUBST(PULSEAUDIO_CFLAGS)
AC_SUBST(PULSEAUDIO_LIBS)

PKG_CHECK_MODULES(SOUNDSERVICE, dbusmenu-glib >= $DBUSMENUGLIB_REQUIRED_VERSION
                                indicator >= $INDICATOR_REQUIRED_VERSION
																indicate >= $INDICATE_REQUIRED_VERSION
																gee-1.0)

AC_SUBST(SOUNDSERVICE_CFLAGS)
AC_SUBST(SOUNDERVICE_LIBS)

AC_PATH_PROG(GCONFTOOL, gconftool-2)
dnl Specify the gconf configuration source,
dnl default to xml::$(sysconfdir)/gconf/gconf.xml.defaults

AM_GCONF_SOURCE_2

PKG_CHECK_MODULES(GCONF, gconf-2.0 >= 2.0)
AC_SUBST(GCONF_CFLAGS)
AC_SUBST(GCONF_LIBS)

###########################
# Check to see if we're local
###########################

with_localinstall="no"
AC_ARG_ENABLE(localinstall, AS_HELP_STRING([--enable-localinstall], [install all of the files localy instead of system directories (for distcheck)]), with_localinstall=$enableval, with_localinstall=no)

###########################
# Indicator Info
###########################

if test "x$with_localinstall" = "xyes"; then
	INDICATORDIR="${libdir}/indicators/2/"
	INDICATORICONSDIR="${datadir}/indicator-applet/icons/"
else
	INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator`
	INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator`
fi
AC_SUBST(INDICATORDIR)
AC_SUBST(INDICATORICONSDIR)

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
tests/Makefile
po/Makefile.in
])

###########################
# Results
###########################

AC_MSG_NOTICE([

SUS Indicator Configuration:

	Prefix:        $prefix
])
