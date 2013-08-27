
include(UseGSettings)

###########################
# Indicator service
###########################

set(
  INDICATOR_DIR
  "${CMAKE_INSTALL_DATADIR}/unity/indicators"
  CACHE FILEPATH "Indicator directory"
)

install(
  FILES "com.canonical.indicator.sound"
  DESTINATION "${INDICATOR_DIR}"
)

###########################
# Dbus Service
###########################

set(
  INDICATOR_SOUND_CONF
  "${CMAKE_CURRENT_BINARY_DIR}/indicator-sound.conf"
)

configure_file(
  "indicator-sound.conf.in"
  ${INDICATOR_SOUND_CONF}
  @ONLY
)

install(
  FILES "${INDICATOR_SOUND_CONF}"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/upstart/sessions/"
)

###########################
# GSettings
###########################

add_schema ("com.canonical.indicator.sound.gschema.xml")
