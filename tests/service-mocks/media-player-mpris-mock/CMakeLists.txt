set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fPIC -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -fno-strict-aliasing -Wextra -fPIC -pthread")

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
    ubuntu::indicators::testing::MediaPlayerMprisMock
    MediaPlayerMprisMockAdaptor)

qt5_add_dbus_adaptor(adaptor_files
    org.mpris.MediaPlayer2.xml
    MediaPlayerMprisMock.h
    ubuntu::indicators::testing::MediaPlayerMprisMock
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

qt5_use_modules(
    media-player-mpris-mock
    Core
    DBus
)

qt5_use_modules(
    media-player-mpris-mock-update
    Core
    DBus
)

# test players desktop files
add_custom_command (OUTPUT testplayers
                    DEPENDS ${CMAKE_SOURCE_DIR}/tests/service-mocks/media-player-mpris-mock/applications
                    COMMAND mkdir -p ${SCHEMA_DIR}
                    COMMAND cp -r ${CMAKE_SOURCE_DIR}/tests/service-mocks/media-player-mpris-mock/applications ${SCHEMA_DIR})
