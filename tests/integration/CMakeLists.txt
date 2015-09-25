set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(FindGMock)

#pkg_check_modules(GMENUHARNESS REQUIRED libgmenuharness REQUIRED)
#include_directories(${GMENUHARNESS_INCLUDE_DIRS})
include_directories("${CMAKE_SOURCE_DIR}/include")

pkg_check_modules(QTDBUSTEST REQUIRED libqtdbustest-1 REQUIRED)
include_directories(${QTDBUSTEST_INCLUDE_DIRS})

pkg_check_modules(QTDBUSMOCK REQUIRED libqtdbusmock-1 REQUIRED)
include_directories(${QTDBUSMOCK_INCLUDE_DIRS})

find_package(Qt5Test REQUIRED)
include_directories(${Qt5Test_INCLUDE_DIRS})

find_package(Qt5DBus REQUIRED)
include_directories(${Qt5DBus_INCLUDE_DIRS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${GMOCK_INCLUDE_DIRS})
include_directories(${GTEST_INCLUDE_DIRS})

include_directories("${CMAKE_SOURCE_DIR}/tests/dbus-types")
include_directories("${CMAKE_BINARY_DIR}/tests/dbus-types")

add_definitions(-DSOUND_SERVICE_BIN="${CMAKE_BINARY_DIR}/src/indicator-sound-service"
                -DSTREAM_RESTORE_TABLE="${CMAKE_SOURCE_DIR}/tests/integration/touch-stream-restore.table"
                -DVOLUME_SET_BIN="${CMAKE_BINARY_DIR}/tests/integration/set-volume"
                -DACCOUNTS_SERVICE_BIN="${CMAKE_BINARY_DIR}/tests/service-mocks/accounts-mock/accounts-service-sound"
                -DMEDIA_PLAYER_MPRIS_BIN="${CMAKE_BINARY_DIR}/tests/service-mocks/media-player-mpris-mock/media-player-mpris-mock"
                -DMEDIA_PLAYER_MPRIS_UPDATE_BIN="${CMAKE_BINARY_DIR}/tests/service-mocks/media-player-mpris-mock/media-player-mpris-mock-update"
                -DTEST_SOUND="${CMAKE_SOURCE_DIR}/tests/integration/test-sound.wav"
                -DQT_NO_KEYWORDS=1
                -DXDG_DATA_DIRS="${XDG_DATA_DIRS}"
)

set(GLIB_REQUIRED_VERSION 2.26)

pkg_check_modules(
  GLIB REQUIRED
  glib-2.0>=${GLIB_REQUIRED_VERSION}
  gio-2.0>=${GLIB_REQUIRED_VERSION}
)
include_directories(${GLIB_INCLUDE_DIRS})

set(
    INTEGRATION_TESTS_SRC
    indicator-sound-test-base.cpp
    test-indicator.cpp
    utils/dbus-pulse-volume.cpp
    main.cpp
)

add_executable(
    integration-tests
    ${INTEGRATION_TESTS_SRC}
)

qt5_use_modules(
    integration-tests
    Core
    DBus
    Test
)

target_link_libraries(
    integration-tests
    sound-indicator-dbus-interfaces
    ${QTDBUSMOCK_LDFLAGS}
    ${QTDBUSTEST_LDFLAGS}
    ${GTEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
#    ${GMENUHARNESS_LDFLAGS}
    ${GLIB_LDFLAGS}
    gmenuharness-shared
)

add_test(
    integration-tests
    integration-tests
)

set(
    SET-VOLUME-SRC
    utils/dbus-pulse-volume.cpp
    utils/set-volume.cpp
)

set(
    GET-VOLUME-SRC
    utils/dbus-pulse-volume.cpp
    utils/get-volume.cpp
)

add_executable(
    set-volume
    ${SET-VOLUME-SRC}
)

add_executable(
    get-volume
    ${GET-VOLUME-SRC}
)

qt5_use_modules(
    set-volume
    Core
    DBus
    Test
)

qt5_use_modules(
    get-volume
    Core
    DBus
    Test
)

target_link_libraries(
    get-volume
    sound-indicator-dbus-interfaces
)

target_link_libraries(
    set-volume
    sound-indicator-dbus-interfaces
)

#add_subdirectory(utils)