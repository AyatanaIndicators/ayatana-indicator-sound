
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
# Upstart Job
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
# XDG Autostart
###########################

set(
  INDICATOR_SOUND_XDG_AUTOSTART
  "${CMAKE_CURRENT_BINARY_DIR}/indicator-sound.desktop"
)

configure_file(
  "indicator-sound.desktop.in"
  ${INDICATOR_SOUND_XDG_AUTOSTART}
  @ONLY
)

install(
  FILES "${INDICATOR_SOUND_XDG_AUTOSTART}"
  DESTINATION "/etc/xdg/autostart"
)

###########################
# Upstart XDG Autostart Override
###########################

set(
  INDICATOR_SOUND_UPSTART_XDG_AUTOSTART
  "${CMAKE_CURRENT_BINARY_DIR}/indicator-sound.upstart.desktop"
)

configure_file(
  "indicator-sound.upstart.desktop.in"
  ${INDICATOR_SOUND_UPSTART_XDG_AUTOSTART}
  @ONLY
)

install(
  FILES "${INDICATOR_SOUND_UPSTART_XDG_AUTOSTART}"
  DESTINATION "${CMAKE_INSTALL_DATADIR}/upstart/xdg/autostart"
  RENAME "indicator-sound.desktop"
)

###########################
# GSettings
###########################

add_schema ("com.canonical.indicator.sound.gschema.xml")
