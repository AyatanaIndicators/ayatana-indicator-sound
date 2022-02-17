
find_package(GSettings)

###########################
# Indicator service
###########################

set(
  INDICATOR_DIR
  "${CMAKE_INSTALL_FULL_DATADIR}/ayatana/indicators"
  CACHE FILEPATH "Ayatana Indicators Directory"
)

install(
  FILES "org.ayatana.indicator.sound"
  DESTINATION "${INDICATOR_DIR}"
)

###########################
# Systemd Job
###########################

pkg_check_modules(SYSTEMD systemd)
if (${SYSTEMD_FOUND})

  # where to install
  pkg_get_variable(SYSTEMD_USER_DIR systemd systemduserunitdir)
  message (STATUS "${SYSTEMD_USER_DIR} is the systemd user unit file install dir")

  set (SYSTEMD_USER_NAME "${CMAKE_PROJECT_NAME}.service")
  set (SYSTEMD_USER_FILE "${CMAKE_CURRENT_BINARY_DIR}/${SYSTEMD_USER_NAME}")
  set (SYSTEMD_USER_FILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/${SYSTEMD_USER_NAME}.in")

  # build it
  configure_file ("${SYSTEMD_USER_FILE_IN}" "${SYSTEMD_USER_FILE}")

  # install it
  install (FILES "${SYSTEMD_USER_FILE}"
           DESTINATION "${SYSTEMD_USER_DIR}")
endif()

###########################
# XDG Autostart
###########################

set(
  INDICATOR_SOUND_XDG_AUTOSTART
  "${CMAKE_CURRENT_BINARY_DIR}/ayatana-indicator-sound.desktop"
)

configure_file(
  "ayatana-indicator-sound.desktop.in"
  ${INDICATOR_SOUND_XDG_AUTOSTART}
  @ONLY
)

install(
  FILES "${INDICATOR_SOUND_XDG_AUTOSTART}"
  DESTINATION "/etc/xdg/autostart"
)

###########################
# GSettings
###########################

add_schema ("org.ayatana.indicator.sound.gschema.xml")

#########################################
# Accounts Service Iface: Sound Indicator
#########################################


set(POLKIT_LIB_DIR "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/lib/polkit-1")
set(POLKIT_DATA_DIR "${CMAKE_INSTALL_FULL_DATADIR}/polkit-1")
set(DBUS_IFACE_DIR "${CMAKE_INSTALL_FULL_DATADIR}/dbus-1/interfaces")
set(ACCOUNTS_IFACE_DIR "${CMAKE_INSTALL_FULL_DATADIR}/accountsservice/interfaces")

install(FILES org.ayatana.indicator.sound.AccountsService.xml
    DESTINATION "${DBUS_IFACE_DIR}"
)

# Create accountsservice symlink for above dbus interface
install(CODE "
    execute_process(COMMAND mkdir -p \"\$ENV{DESTDIR}${ACCOUNTS_IFACE_DIR}\")
    execute_process(COMMAND ln -sf ../../dbus-1/interfaces/org.ayatana.indicator.sound.AccountsService.xml \"\$ENV{DESTDIR}${ACCOUNTS_IFACE_DIR}\")
")

install(FILES org.ayatana.indicator.sound.AccountsService.policy
DESTINATION "${POLKIT_DATA_DIR}/actions"
)

install(FILES 50-org.ayatana.indicator.sound.AccountsService.pkla
DESTINATION "${POLKIT_LIB_DIR}/localauthority/10-vendor.d"
)
