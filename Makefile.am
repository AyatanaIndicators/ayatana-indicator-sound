
SUBDIRS = \
	src \
	data \
	po

EXTRA_DIST = \
  autogen.sh \
  vapi/common-defs.vapi \
  vapi/config.vapi

DISTCHECK_CONFIGURE_FLAGS = --enable-localinstall

include $(top_srcdir)/Makefile.am.coverage
