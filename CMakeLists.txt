project(ayatana-indicator-sound C CXX)
cmake_minimum_required(VERSION 3.13)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "..." FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")

set(PACKAGE ${CMAKE_PROJECT_NAME})
set(PROJECT_VERSION 0.9.0)
find_package(PkgConfig REQUIRED)
include(GNUInstallDirs)
include(UseVala)

# Options
option(ENABLE_TESTS "Enable all tests and checks" OFF)
option(ENABLE_COVERAGE "Enable coverage reports (includes enabling all tests and checks)" OFF)
option(ENABLE_WERROR "Treat all build warnings as errors" OFF)

if(ENABLE_COVERAGE)
    set(ENABLE_TESTS ON)
    set(CMAKE_BUILD_TYPE "Coverage")
else()
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(ENABLE_WERROR)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()

set(GETTEXT_PACKAGE "ayatana-indicator-sound")
set(LOCALEDIR "${CMAKE_INSTALL_FULL_LOCALEDIR}")
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

pkg_check_modules(
  SOUNDSERVICE REQUIRED
  gee-0.8
  gio-2.0>=${GIO_2_0_REQUIRED_VERSION}
  gio-unix-2.0
  gthread-2.0
  libxml-2.0
  libnotify
  accountsservice
  libayatana-common
)
include_directories(${SOUNDSERVICE_INCLUDE_DIRS})

pkg_check_modules(LOMIRI_API liblomiri-api>=0.1.1)
pkg_check_modules(LOMIRI_SCHEMAS lomiri-schemas)

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

if (ENABLE_TESTS)
    pkg_check_modules(TEST REQUIRED dbustest-1>=15.04.0)
    enable_testing()
    add_subdirectory(tests)
    if (ENABLE_COVERAGE)
        find_package(CoverageReport)
        ENABLE_COVERAGE_REPORT(
            TARGETS ${COVERAGE_TARGETS}
            TESTS ${COVERAGE_TEST_TARGETS}
            FILTER /usr/include ${CMAKE_BINARY_DIR}/*
        )
    endif()
endif ()

# Display config info

message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Unit tests: ${ENABLE_TESTS}")
message(STATUS "Build with -Werror: ${ENABLE_WERROR}")
