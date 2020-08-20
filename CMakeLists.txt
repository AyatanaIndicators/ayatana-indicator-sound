project(ayatana-indicator-sound C CXX)
cmake_minimum_required(VERSION 2.8.9)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

set(PACKAGE ${CMAKE_PROJECT_NAME})
set(PROJECT_VERSION 0.8.0)
find_package(PkgConfig REQUIRED)
include(GNUInstallDirs)
include(UseVala)

option (enable_tests "Build the package's automatic tests." ON)

set(GETTEXT_PACKAGE "ayatana-indicator-sound")
set(LOCALEDIR "${CMAKE_INSTALL_FULL_DATADIR}/locale")
add_definitions( -DGETTEXT_PACKAGE="${GETTEXT_PACKAGE}" )

set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(SOURCE_BINARY_DIR "${CMAKE_BINARY_DIR}/src")

set(PULSE_AUDIO_REQUIRED_VERSION 0.9.19)
set(GLIB_2_0_REQUIRED_VERSION 2.32)
set(GIO_2_0_REQUIRED_VERSION 2.25.13)

pkg_check_modules(
  PULSEAUDIO REQUIRED
  libpulse-mainloop-glib>=${PULSE_AUDIO_REQUIRED_VERSION}
  glib-2.0>=${GLIB_2_0_REQUIRED_VERSION}
  gio-unix-2.0>=${GIO_2_0_REQUIRED_VERSION}
)
include_directories(${PULSEAUDIO_INCLUDE_DIRS})

set(URL_DISPATCHER_1_REQUIRED_VERSION 1)

pkg_check_modules(
  URLDISPATCHER
  url-dispatcher-1>=${URL_DISPATCHER_1_REQUIRED_VERSION}
)
include_directories(${URLDISPATCHER_INCLUDE_DIRS})

pkg_check_modules(
  SOUNDSERVICE REQUIRED
  gee-0.8
  gio-2.0>=${GIO_2_0_REQUIRED_VERSION}
  gio-unix-2.0
  gthread-2.0
  libxml-2.0
  libnotify
  accountsservice
)
include_directories(${SOUNDSERVICE_INCLUDE_DIRS})

pkg_check_modules(UNITY_API libunity-api>=0.1.3)

include_directories(${TEST_INCLUDE_DIRS})

find_package(Vala 0.20)

find_package(GObjectIntrospection 0.9.12)

include_directories(${SOURCE_DIR})
include_directories(${SOURCE_BINARY_DIR})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_definitions(
  -Wall
)

configure_file(
  "config.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/config.h"
)

add_subdirectory(data)
add_subdirectory(src)
add_subdirectory(po)

if (${enable_tests})
    pkg_check_modules(TEST REQUIRED dbustest-1>=15.04.0)
    enable_testing()
    add_subdirectory(tests)
    find_package(CoverageReport)
    ENABLE_COVERAGE_REPORT(
        TARGETS ${COVERAGE_TARGETS}
        TESTS ${COVERAGE_TEST_TARGETS}
        FILTER /usr/include ${CMAKE_BINARY_DIR}/*
 )
endif ()
