
###########################
# Vala Generation
###########################

set(HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/indicator-sound-service.h")
set(SYMBOLS_PATH "${CMAKE_CURRENT_BINARY_DIR}/indicator-sound-service.def")

vala_init(indicator-sound-service
  PACKAGES
    config
    gio-2.0
    gio-unix-2.0
    libxml-2.0
    libpulse
    libpulse-mainloop-glib
  OPTIONS
    --ccode
    --thread
    --vapidir=${CMAKE_SOURCE_DIR}/vapi/
    --vapidir=.
    --target-glib=2.36
)

vala_add(indicator-sound-service
  service.vala
  DEPENDS
    sound-menu
    volume-control
    media-player
    media-player-list
)
vala_add(indicator-sound-service
  main.vala
  DEPENDS
    service
)
vala_add(indicator-sound-service
  volume-control.vala
)
vala_add(indicator-sound-service
  media-player.vala
  DEPENDS
    mpris2-interfaces
)
vala_add(indicator-sound-service
  media-player-list.vala
  DEPENDS
    media-player
    mpris2-watcher
)
vala_add(indicator-sound-service
  mpris2-interfaces.vala
)
vala_add(indicator-sound-service
  mpris2-watcher.vala
  DEPENDS
    media-player
    mpris2-interfaces
    freedesktop-interfaces
)
vala_add(indicator-sound-service
  freedesktop-interfaces.vala
)
vala_add(indicator-sound-service
  sound-menu.vala
  DEPENDS
    media-player
)

vala_finish(indicator-sound-service
  SOURCES
    project_VALA_SOURCES
  OUTPUTS
    project_VALA_C
  GENERATE_HEADER
    ${HEADER_PATH}
  GENERATE_SYMBOLS
    ${SYMBOLS_PATH}
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
  ${SYMBOLS_PATH}
)

###########################
# Executable
###########################

add_definitions(
  -w
)

add_executable(
  indicator-sound-service-bin
  ${INDICATOR_SOUND_SOURCES}
)

set_target_properties(
  indicator-sound-service-bin
  PROPERTIES
  OUTPUT_NAME "indicator-sound-service"
)

target_link_libraries(
  indicator-sound-service-bin
  ${PULSEAUDIO_LIBRARIES}
  ${SOUNDSERVICE_LIBRARIES}
)

###########################
# Installation
###########################

install(
  TARGETS indicator-sound-service-bin
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}
)

