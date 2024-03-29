set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_definitions("-fPIC")

find_package(Qt5DBus REQUIRED)
include_directories(${Qt5DBus_INCLUDE_DIRS}
                    "${CMAKE_SOURCE_DIR}/tests/service-mocks")

add_definitions(-DQT_NO_KEYWORDS=1)

set(dbusinterface_mediaplayermpris_xml "org.mpris.MediaPlayer2.Player.xml")
set_source_files_properties(${dbusinterface_mediaplayermpris_xml} PROPERTIES
  CLASSNAME MediaPlayerMprisMockInterface)

qt5_add_dbus_interface(interface_files ${dbusinterface_mediaplayermpris_xml} MediaPlayerMprisMockInterface)

qt5_add_dbus_adaptor(adaptor_files
    org.mpris.MediaPlayer2.Player.xml
    MediaPlayerMprisMock.h
    ayatana::indicators::testing::MediaPlayerMprisMock
    MediaPlayerMprisMockAdaptor)

qt5_add_dbus_adaptor(adaptor_files
    org.mpris.MediaPlayer2.xml
    MediaPlayerMprisMock.h
    ayatana::indicators::testing::MediaPlayerMprisMock
    MediaPlayer2MockAdaptor)

add_executable(
  media-player-mpris-mock
  ${adaptor_files}
  MediaPlayerMprisMock.cpp
  ${CMAKE_SOURCE_DIR}/tests/service-mocks/DBusPropertiesNotifier.cpp
  main.cpp
  testplayers
)

add_executable(
  media-player-mpris-mock-update
  ${interface_files}
  player-update.cpp
  testplayers
)

target_link_libraries(
    media-player-mpris-mock
    Qt5::Core
    Qt5::DBus
)

target_link_libraries(
    media-player-mpris-mock-update
    Qt5::Core
    Qt5::DBus
)

# test players desktop files
add_custom_command (OUTPUT testplayers
                    DEPENDS ${CMAKE_SOURCE_DIR}/tests/service-mocks/media-player-mpris-mock/applications
                    COMMAND mkdir -p ${XDG_DATA_DIRS}
                    COMMAND cp -r ${CMAKE_SOURCE_DIR}/tests/service-mocks/media-player-mpris-mock/applications ${XDG_DATA_DIRS})
