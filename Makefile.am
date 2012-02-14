
SUBDIRS = \
	src \
	data \
	po

EXTRA_DIST = \
  autogen.sh \
  vapi/common-defs.vapi \
  vapi/config.vapi

DISTCHECK_CONFIGURE_FLAGS = --enable-localinstall


# Coverage targets

.PHONY: clean-gcda
clean-gcda:
	@echo Removing old coverage results
	-find -name '*.gcda' -print | xargs -r rm

.PHONY: coverage-html generate-coverage-html clean-coverage-html
coverage-html: clean-gcda
	-$(MAKE) $(AM_MAKEFLAGS) -k check
	$(MAKE) $(AM_MAKEFLAGS) generate-coverage-html

generate-coverage-html:
	@echo Collecting coverage data
	$(LCOV) --directory $(top_builddir) --capture --output-file coverage.info --no-checksum --compat-libtool
	LANG=C $(GENHTML) --prefix $(top_builddir) --output-directory coveragereport --title "Code Coverage" --legend --show-details coverage.info

clean-coverage-html: clean-gcda
	-$(LCOV) --directory $(top_builddir) -z
	-rm -rf coverage.info coveragereport

.PHONY: coverage-xml generate-coverage-xml clean-coverage-xml
coverage-xml: clean-gcda
	-$(MAKE) $(AM_MAKEFLAGS) -k check
	$(MAKE) $(AM_MAKEFLAGS) generate-coverage-xml

generate-coverage-xml:
	@echo Generating coverage XML report
	$(GCOVR) -x -r $(top_builddir) -o $(top_builddir)/coverage.xml

clean-coverage-xml: clean-gcda
	-rm -rf $(top_builddir)/coverage.xml

clean-local: clean-coverage-html clean-coverage-xml
