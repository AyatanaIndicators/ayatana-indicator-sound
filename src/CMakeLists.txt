###########################
# Vala Generation
###########################

set(HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/indicator-sound-service.h")
set(SYMBOLS_PATH "${CMAKE_CURRENT_BINARY_DIR}/indicator-sound-service.def")
set(VAPI_PATH "${CMAKE_CURRENT_BINARY_DIR}/indicator-sound-service.vapi")

if(URLDISPATCHER_FOUND)
    set (VALA_DEFINE_URLDISPATCHER "--define=HAS_URLDISPATCHER")
    set (VALA_PKG_URLDISPATCHER "lomiri-url-dispatcher")
endif()

if(EXISTS "/usr/share/glib-2.0/schemas/com.ubuntu.sound.gschema.xml")
  set (VALA_DEFINE_UBUNTU_SOUND_SCHEMA "--define=HAS_UBUNTU_SOUND_SCHEMA")
endif()

if(EXISTS "/usr/share/accountsservice/interfaces/com.ubuntu.touch.AccountsService.SecurityPrivacy.xml")
  set (HAVE_UT_ACCTSERVICE_PRIVACY_SETTINGS ON)
  set (VALA_FILE_ACCTSERVICE_PRIVACY_SETTINGS "accounts-service-privacy-settings")
  set (VALA_DEFINE_ACCTSERVICE_PRIVACY_SETTINGS "--define=HAS_UT_ACCTSERVICE_PRIVACY_SETTINGS")
else()
  set (HAVE_UT_ACCTSERVICE_PRIVACY_SETTINGS OFF)
endif()

if(EXISTS "/usr/share/accountsservice/interfaces/com.ubuntu.touch.AccountsService.Sound.xml")
  set (HAVE_UT_ACCTSERVICE_SYSTEMSOUND_SETTINGS ON)
  set (VALA_FILE_ACCTSERVICE_SYSTEMSOUND_SETTINGS "accounts-service-system-sound-settings")
  set (VALA_DEFINE_ACCTSERVICE_SYSTEMSOUND_SETTINGS "--define=HAS_UT_ACCTSERVICE_SYSTEMSOUND_SETTINGS")
else()
  set (HAVE_UT_ACCTSERVICE_SYSTEMSOUND_SETTINGS OFF)
endif()

if(EXISTS "/usr/share/accountsservice/interfaces/com.ubuntu.AccountsService.Sound.xml")
  set (HAVE_UT_ACCTSERVICE_SOUND_SETTINGS ON)
  set (VALA_DEFINE_ACCTSERVICE_SOUND_SETTINGS "--define=HAS_UT_ACCTSERVICE_SOUND_SETTINGS")
else()
  set (HAVE_UT_ACCTSERVICE_SOUND_SETTINGS OFF)
endif()

vapi_gen(accounts-service
    LIBRARY
        accounts-service
    PACKAGES
        gio-2.0
    INPUT
        /usr/share/gir-1.0/AccountsService-1.0.gir
)

vala_init(ayatana-indicator-sound-service
  DEPENDS
    accounts-service
  PACKAGES
    config
    gee-0.8
    gio-2.0
    gio-unix-2.0
    libxml-2.0
    libpulse
    libpulse-mainloop-glib
    libnotify
    accounts-service
    ${VALA_PKG_URLDISPATCHER}
  OPTIONS
    --ccode
    --thread
    --target-glib=${GLIB_2_0_REQUIRED_VERSION}
    --vapidir=${CMAKE_SOURCE_DIR}/vapi/
    --vapidir=.
    --pkg=bus-watcher
    ${VALA_DEFINE_URLDISPATCHER}
    ${VALA_DEFINE_UBUNTU_SOUND_SCHEMA}
    ${VALA_DEFINE_ACCTSERVICE_PRIVACY_SETTINGS}
    ${VALA_DEFINE_ACCTSERVICE_SOUND_SETTINGS}
    ${VALA_DEFINE_ACCTSERVICE_SYSTEMSOUND_SETTINGS}
)

vala_add(ayatana-indicator-sound-service
  notification.vala
)
vala_add(ayatana-indicator-sound-service
  info-notification.vala
  DEPENDS
    notification
      volume-control
        options
)
vala_add(ayatana-indicator-sound-service
  warn-notification.vala
  DEPENDS
    notification
)
if(URLDISPATCHER_FOUND)
    vala_add(ayatana-indicator-sound-service
      service.vala
      DEPENDS
        sound-menu
        volume-control
        volume-control-pulse
        notification
        info-notification
        volume-warning
        options
        options-gsettings
        media-player
        media-player-list
        mpris2-interfaces
        accounts-service-user
        accounts-service-access
    )
else()
    vala_add(ayatana-indicator-sound-service
      service.vala
      DEPENDS
        sound-menu
        volume-control
        volume-control-pulse
        notification
        info-notification
        volume-warning
        options
        options-gsettings
        media-player
        media-player-list
        mpris2-interfaces
        accounts-service-user
        accounts-service-access
    )
endif()

vala_add(ayatana-indicator-sound-service
  options.vala
  DEPENDS
    volume-control
    volume-control-pulse
    accounts-service-access
)
vala_add(ayatana-indicator-sound-service
  options-gsettings.vala
  DEPENDS
    options
    volume-control-pulse
    volume-control
    accounts-service-access
)
vala_add(ayatana-indicator-sound-service
  volume-control.vala
  DEPENDS
    options
    volume-control-pulse
    accounts-service-access
)
vala_add(ayatana-indicator-sound-service
  accounts-service-access.vala
)
vala_add(ayatana-indicator-sound-service
  volume-control-pulse.vala
  DEPENDS
    accounts-service-access
    options
    volume-control
)
vala_add(ayatana-indicator-sound-service
  volume-warning.vala
  DEPENDS
    options
    volume-control-pulse
    volume-control
    warn-notification
    notification
    accounts-service-access
)
vala_add(ayatana-indicator-sound-service
  volume-warning-pulse.vala
  DEPENDS
    volume-warning
    options
    volume-control-pulse
    volume-control
    warn-notification
    notification
    accounts-service-access
)
vala_add(ayatana-indicator-sound-service
  media-player.vala
)
vala_add(ayatana-indicator-sound-service
  media-player-mpris.vala
  DEPENDS
    media-player
    mpris2-interfaces
)
vala_add(ayatana-indicator-sound-service
  media-player-user.vala
  DEPENDS
    media-player
    accounts-service-sound-settings
    greeter-broadcast
)
vala_add(ayatana-indicator-sound-service
  media-player-list.vala
  DEPENDS
    media-player
)
vala_add(ayatana-indicator-sound-service
  media-player-list-mpris.vala
  DEPENDS
    media-player-list
    media-player
    media-player-mpris
    mpris2-interfaces
)
vala_add(ayatana-indicator-sound-service
  media-player-list-greeter.vala
  DEPENDS
    media-player-list
    media-player-user
    media-player
)
vala_add(ayatana-indicator-sound-service
  mpris2-interfaces.vala
)
vala_add(ayatana-indicator-sound-service
  freedesktop-interfaces.vala
)
vala_add(ayatana-indicator-sound-service
  sound-menu.vala
  DEPENDS
    media-player
    volume-control
    options
    volume-control-pulse
    accounts-service-access
)
vala_add(ayatana-indicator-sound-service
  accounts-service-user.vala
  DEPENDS
    media-player
    mpris2-interfaces
    accounts-service-sound-settings
    ${VALA_FILE_ACCTSERVICE_PRIVACY_SETTINGS}
    ${VALA_FILE_ACCTSERVICE_SYSTEMSOUND_SETTINGS}
    greeter-broadcast
)
vala_add(ayatana-indicator-sound-service
  accounts-service-sound-settings.vala
)

if(${HAVE_UT_ACCTSERVICE_PRIVACY_SETTINGS})
    vala_add(ayatana-indicator-sound-service
          accounts-service-system-sound-settings.vala
    )
endif()

if(${HAVE_UT_ACCTSERVICE_SYSTEMSOUND_SETTINGS})
    vala_add(ayatana-indicator-sound-service
          accounts-service-privacy-settings.vala
    )
endif()

vala_add(ayatana-indicator-sound-service
  greeter-broadcast.vala
)

vala_finish(ayatana-indicator-sound-service
  SOURCES
    project_VALA_SOURCES
  OUTPUTS
    project_VALA_C
  GENERATE_HEADER
    ${HEADER_PATH}
  GENERATE_SYMBOLS
    ${SYMBOLS_PATH}
  GENERATE_VAPI
    ${VAPI_PATH}
)

set_source_files_properties(
  ${project_VALA_SOURCES}
  PROPERTIES
  HEADER_FILE_ONLY TRUE
)

set(
  INDICATOR_SOUND_SOURCES
  ${project_VALA_SOURCES}
  ${project_VALA_C}
  bus-watch-namespace.c
  ${SYMBOLS_PATH}
)

###########################
# Lib
###########################

add_definitions(
  -w
  -DG_LOG_DOMAIN="ayatana-indicator-sound"
)

add_library(
    indicator-sound-service-lib STATIC
    ${INDICATOR_SOUND_SOURCES}
)

target_link_libraries(
  indicator-sound-service-lib
  ${SOUNDSERVICE_LIBRARIES}
  ${URLDISPATCHER_LIBRARIES}
  -lm
)

###########################
# Executable
###########################

include_directories(${CMAKE_BINARY_DIR})

add_executable(
  indicator-sound-service-bin
  main.c
)

set_target_properties(
  indicator-sound-service-bin
  PROPERTIES
  OUTPUT_NAME "ayatana-indicator-sound-service"
)

target_link_libraries(
  indicator-sound-service-bin
  indicator-sound-service-lib
  ${PULSEAUDIO_LIBRARIES}
)

###########################
# Installation
###########################

install(
  TARGETS indicator-sound-service-bin
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/ayatana-indicator-sound/
)

if(UNITY_API_FOUND)
add_subdirectory(gmenuharness)
endif()

# coverage reporting
set(COVERAGE_TARGETS
  ${COVERAGE_TARGETS}
  indicator-sound-service-lib
  indicator-sound-service-bin
  PARENT_SCOPE
)
