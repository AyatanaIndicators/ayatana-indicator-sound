
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
  INDICATOR_SOUND_SERVICE
  "${CMAKE_CURRENT_BINARY_DIR}/indicator-sound.service"
)

configure_file(
  "indicator-sound.service.in"
  ${INDICATOR_SOUND_SERVICE}
  @ONLY
)

install(
  FILES "${INDICATOR_SOUND_SERVICE}"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/dbus-1/services/"
)

###########################
# GSettings
###########################

add_schema ("com.canonical.indicator.sound.gschema.xml")
