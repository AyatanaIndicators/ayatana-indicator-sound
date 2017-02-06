
find_package(GSettings)

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
# Systemd Job
###########################

# where to install
# Uncomment when we drop Vivid
# pkg_get_variable(SYSTEMD_USER_DIR systemd systemduserunitdir)
set (SYSTEMD_USER_DIR "/usr/lib/systemd/user")
message (STATUS "${SYSTEMD_USER_DIR} is the systemd user unit file install dir")

set (SYSTEMD_USER_NAME "${CMAKE_PROJECT_NAME}.service")
set (SYSTEMD_USER_FILE "${CMAKE_CURRENT_BINARY_DIR}/${SYSTEMD_USER_NAME}")
set (SYSTEMD_USER_FILE_IN "${CMAKE_CURRENT_SOURCE_DIR}/${SYSTEMD_USER_NAME}.in")

# build it
set (pkglibexecdir "${CMAKE_INSTALL_FULL_PKGLIBEXECDIR}")
configure_file ("${SYSTEMD_USER_FILE_IN}" "${SYSTEMD_USER_FILE}")

# install it
install (FILES "${SYSTEMD_USER_FILE}"
         DESTINATION "${SYSTEMD_USER_DIR}")

###########################
#  Upstart systemd override Job File
###########################

set (UPSTART_SYSTEMD_OVERRIDE_DIR "${CMAKE_INSTALL_FULL_DATADIR}/upstart/systemd-session/upstart")
message (STATUS "${UPSTART_SYSTEMD_OVERRIDE_DIR} is the Upstart override Job File for systemd dir")

install (FILES "${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}.override"
         DESTINATION "${UPSTART_SYSTEMD_OVERRIDE_DIR}")

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
# GSettings
###########################

add_schema ("com.canonical.indicator.sound.gschema.xml")

###########################
# Accounts Service
###########################


set(POLKIT_LIB_DIR "${CMAKE_INSTALL_LOCALSTATEDIR}/lib/polkit-1")
set(POLKIT_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/polkit-1")
set(DBUS_IFACE_DIR "${CMAKE_INSTALL_PREFIX}/share/dbus-1/interfaces")
set(ACCOUNTS_IFACE_DIR "${CMAKE_INSTALL_PREFIX}/share/accountsservice/interfaces")

install(FILES com.canonical.indicator.sound.AccountsService.xml
	DESTINATION "${DBUS_IFACE_DIR}"
)

# Create accountsservice symlink for above dbus interface
install(CODE "
	execute_process(COMMAND mkdir -p \"\$ENV{DESTDIR}${ACCOUNTS_IFACE_DIR}\")
	execute_process(COMMAND ln -sf ../../dbus-1/interfaces/com.canonical.indicator.sound.AccountsService.xml \"\$ENV{DESTDIR}${ACCOUNTS_IFACE_DIR}\")
")

install(FILES com.canonical.indicator.sound.AccountsService.policy
DESTINATION "${POLKIT_DATA_DIR}/actions"
)

install(FILES 50-com.canonical.indicator.sound.AccountsService.pkla
DESTINATION "${POLKIT_LIB_DIR}/localauthority/10-vendor.d"
)




