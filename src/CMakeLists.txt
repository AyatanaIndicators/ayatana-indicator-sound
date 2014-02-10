
###########################
# Vala Generation
###########################

set(HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/indicator-sound-service.h")
set(SYMBOLS_PATH "${CMAKE_CURRENT_BINARY_DIR}/indicator-sound-service.def")

vapi_gen(accounts-service
	LIBRARY
		accounts-service
	PACKAGES
		gio-2.0
	INPUT
		/usr/share/gir-1.0/AccountsService-1.0.gir
)

vala_init(indicator-sound-service
  DEPENDS
    accounts-service
  PACKAGES
    config
    gio-2.0
    gio-unix-2.0
    libxml-2.0
    libpulse
    libpulse-mainloop-glib
    libnotify
    accounts-service
  OPTIONS
    --ccode
    --thread
    --vapidir=${CMAKE_SOURCE_DIR}/vapi/
    --vapidir=.
    --pkg=url-dispatcher
    --pkg=bus-watcher
)

vala_add(indicator-sound-service
  service.vala
  DEPENDS
    sound-menu
    volume-control
    media-player
    media-player-list
    mpris2-interfaces
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
    mpris2-interfaces
)
vala_add(indicator-sound-service
  mpris2-interfaces.vala
)
vala_add(indicator-sound-service
  freedesktop-interfaces.vala
)
vala_add(indicator-sound-service
  sound-menu.vala
  DEPENDS
    media-player
    mpris2-interfaces
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
  bus-watch-namespace.c
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
  RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/indicator-sound/
)

