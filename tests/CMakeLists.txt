find_package(GMock)
include_directories(${GMOCK_INCLUDE_DIRS})
include_directories("${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}/libdbustest-1")

###########################
# GSettings Schema
###########################

# build the necessary schemas
set_directory_properties (PROPERTIES
                          ADDITIONAL_MAKE_CLEAN_FILES gschemas.compiled)
set_source_files_properties (gschemas.compiled GENERATED)

# the home path needs to be severely below 108 characters of length, as pulseaudio
# will try to place a unix domain socket file in there... (i.e. in XDG_RUNTIME_DIR).
# In previous versions, the test-home was in the build directory somewhere, which failed
# when the source tree was stored in some deep sub/sub/sub/folders structure.
set (TEST_HOME "/tmp/build-home")
set (XDG_CONFIG_HOME "${TEST_HOME}/.config")
set (XDG_RUNTIME_DIR "${TEST_HOME}/.run")

add_custom_command (OUTPUT home.created
                    COMMAND mkdir -p ${TEST_HOME}
                    COMMAND mkdir -p ${XDG_RUNTIME_DIR}
                    COMMAND mkdir -p ${XDG_CONFIG_HOME}
)
add_custom_target (
             test-home ALL DEPENDS home.created
)

# GSettings:
# compile the indicator-sound schema into a gschemas.compiled file in this directory,
# and help the tests to find that file by setting -DSCHEMA_DIR
set (XDG_DATA_DIRS "${CMAKE_CURRENT_BINARY_DIR}/gsettings-schemas")
set (SCHEMA_DIR "${XDG_DATA_DIRS}/glib-2.0/schemas")

if(ENABLE_LOMIRI_FEATURES)
  add_definitions ( -DLOMIRI_FEATURES_ENABLED )
endif()

add_definitions(-DSCHEMA_DIR="${SCHEMA_DIR}")
execute_process (COMMAND ${PKG_CONFIG_EXECUTABLE} gio-2.0 --variable glib_compile_schemas
                 OUTPUT_VARIABLE COMPILE_SCHEMA_EXECUTABLE
                 OUTPUT_STRIP_TRAILING_WHITESPACE)
add_custom_command (OUTPUT gschemas.compiled
                    DEPENDS ${CMAKE_SOURCE_DIR}/data/org.ayatana.indicator.sound.gschema.xml
                    COMMAND mkdir -p ${SCHEMA_DIR}
                    COMMAND cp -f ${CMAKE_SOURCE_DIR}/data/org.ayatana.indicator.sound.gschema.xml ${SCHEMA_DIR}
                    COMMAND test -e /usr/share/glib-2.0/schemas/com.lomiri.sound.gschema.xml && cp -f /usr/share/glib-2.0/schemas/com.lomiri.sound.gschema.xml ${SCHEMA_DIR} || echo "File does not exist" >&2
                    COMMAND ${COMPILE_SCHEMA_EXECUTABLE} ${SCHEMA_DIR})

add_custom_target (
             gschemas-compiled ALL DEPENDS gschemas.compiled
)

###########################
# Vala Mocks
###########################

set(VALA_MOCKS_HEADER_PATH "${CMAKE_CURRENT_BINARY_DIR}/vala-mocks.h")
set(VALA_MOCKS_SYMBOLS_PATH "${CMAKE_CURRENT_BINARY_DIR}/vala-mocks.def")

vala_init(vala-mocks
    DEPENDS
        indicator-sound-service-lib
    PACKAGES
        config
        gio-2.0
        gio-unix-2.0
        libxml-2.0
        libpulse
        libpulse-mainloop-glib
        libnotify
        accounts-service
        indicator-sound-service
    OPTIONS
        --ccode
        --vapidir=${CMAKE_BINARY_DIR}/src/
        --vapidir=${CMAKE_SOURCE_DIR}/vapi/
        --vapidir=.
)

set_source_files_properties(media-player-mock.c PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS} -Wno-incompatible-pointer-types -Wno-discarded-qualifiers")
set_source_files_properties(media-player-list-mock.c PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS} -Wno-incompatible-pointer-types -Wno-unused-variable -Wno-discarded-qualifiers")
set_source_files_properties(options-mock.c PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS} -Wno-incompatible-pointer-types -Wno-unused-variable -Wno-discarded-qualifiers")
set_source_files_properties(volume-control-mock.c PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS} -Wno-incompatible-pointer-types -Wno-discarded-qualifiers")
set_source_files_properties(volume-warning-mock.c PROPERTIES COMPILE_FLAGS "${CMAKE_C_FLAGS} -Wno-incompatible-pointer-types -Wno-unused-variable -Wno-discarded-qualifiers")

vala_add(vala-mocks
    media-player-mock.vala
)

vala_add(vala-mocks
    media-player-list-mock.vala
)

vala_add(vala-mocks
    options-mock.vala
)

vala_add(vala-mocks
    volume-control-mock.vala
)

vala_add(vala-mocks
    volume-warning-mock.vala
)

vala_finish(vala-mocks
    SOURCES
        vala_mocks_VALA_SOURCES
    OUTPUTS
        vala_mocks_VALA_C
    GENERATE_HEADER
        ${VALA_MOCKS_HEADER_PATH}
    GENERATE_SYMBOLS
        ${VALA_MOCKS_SYMBOLS_PATH}
)

set_source_files_properties(
    ${vala_mocks_VALA_SOURCES}
    PROPERTIES
    HEADER_FILE_ONLY TRUE
)

set(
    VALA_MOCKS_SOURCES
    ${vala_mocks_VALA_SOURCES}
    ${vala_mocks_VALA_C}
    ${VALA_MOCKS_SYMBOLS_PATH}
)

add_definitions(
    -Wno-unknown-warning-option
    -Wno-unused-but-set-variable
    -DVALA_EXTERN=extern
)

add_library(
    vala-mocks-lib STATIC
    ${VALA_MOCKS_SOURCES}
)

target_link_libraries(
    vala-mocks-lib
    indicator-sound-service-lib
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${TEST_INCLUDE_DIRS})

###########################
# Pulse Mock
###########################

add_library(
    pulse-mock
    SHARED
    pa-mock.cpp
)

target_link_libraries (pulse-mock ${PULSEAUDIO_LIBRARIES})

###########################
# Name Watch Test
###########################

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable (name-watch-test name-watch-test.cc ${CMAKE_SOURCE_DIR}/src/bus-watch-namespace.c)
target_link_options (name-watch-test PRIVATE -no-pie)
target_link_libraries (name-watch-test ${SOUNDSERVICE_LIBRARIES} ${GMOCK_LIBRARIES})
add_test(name-watch-test name-watch-test)

###########################
# Accounts Service User
###########################

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable (accounts-service-user-test accounts-service-user.cc)
target_link_options (accounts-service-user-test PRIVATE -no-pie)
target_link_libraries (
    accounts-service-user-test
    indicator-sound-service-lib
    vala-mocks-lib
    ${SOUNDSERVICE_LIBRARIES}
    ${TEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

# Split tests to work around libaccountservice sucking
add_test(accounts-service-user-test-basic
    accounts-service-user-test  --gtest_filter=AccountsServiceUserTest.BasicObject
)

add_test(accounts-service-user-test-player
    accounts-service-user-test  --gtest_filter=AccountsServiceUserTest.SetMediaPlayer
)

###########################
# Volume Control
###########################

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable (volume-control-test volume-control-test.cc)
add_dependencies (volume-control-test gschemas-compiled)
target_link_options (volume-control-test PRIVATE -no-pie)
target_link_libraries (
    volume-control-test
    indicator-sound-service-lib
    vala-mocks-lib
    pulse-mock
    ${TEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

add_test(volume-control-test volume-control-test)

###########################
# Sound Menu
###########################

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable (sound-menu-test sound-menu.cc)
target_link_options (sound-menu-test PRIVATE -no-pie)
target_link_libraries (
    sound-menu-test
    indicator-sound-service-lib
    vala-mocks-lib
    ${SOUNDSERVICE_LIBRARIES}
    ${TEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

add_test(sound-menu-test sound-menu-test)

###########################
# Notification Test
###########################

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable (notifications-test notifications-test.cc)
target_link_options (notifications-test PRIVATE -no-pie)
target_link_libraries (
    notifications-test
    indicator-sound-service-lib
    vala-mocks-lib
    pulse-mock
    ${SOUNDSERVICE_LIBRARIES}
    ${TEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

add_test(notifications-test notifications-test)

###########################
# Accounts Service User
###########################

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable (media-player-user-test media-player-user.cc)
target_link_options (media-player-user-test PRIVATE -no-pie)
target_link_libraries (
    media-player-user-test
    indicator-sound-service-lib
    vala-mocks-lib
    ${SOUNDSERVICE_LIBRARIES}
    ${TEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

# Split tests to work around libaccountservice sucking
add_test(media-player-user-test-basic
    media-player-user-test  --gtest_filter=MediaPlayerUserTest.BasicObject
)
add_test(media-player-user-test-dataset
    media-player-user-test  --gtest_filter=MediaPlayerUserTest.DataSet
)
add_test(media-player-user-test-timeout
    media-player-user-test  --gtest_filter=MediaPlayerUserTest.TimeoutTest
)

###########################
# Greeter List
###########################

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable (greeter-list-test greeter-list.cc)
target_link_options (greeter-list-test PRIVATE -no-pie)
target_link_libraries (
    greeter-list-test
    indicator-sound-service-lib
    vala-mocks-lib
    ${SOUNDSERVICE_LIBRARIES}
    ${TEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

# Split tests to work around libaccountservice sucking
add_test(greeter-list-test-basic
    greeter-list-test  --gtest_filter=GreeterListTest.BasicObject
)
add_test(greeter-list-test-iterator
    greeter-list-test  --gtest_filter=GreeterListTest.BasicIterator
)

###########################
# Indicator Test
###########################

add_definitions(
    -DINDICATOR_SOUND_SERVICE_BINARY="${CMAKE_BINARY_DIR}/src/ayatana-indicator-sound-service"
    -DPA_MOCK_LIB="${CMAKE_CURRENT_BINARY_DIR}/libpulse-mock.so"
)
add_executable (indicator-test indicator-test.cc)
add_dependencies (indicator-test gschemas-compiled)
target_link_options (indicator-test PRIVATE -no-pie)
target_link_libraries (
    indicator-test
    ${SOUNDSERVICE_LIBRARIES}
    ${TEST_LIBRARIES}
    ${GMOCK_LIBRARIES}
)

# Split tests to work around libaccountservice sucking
add_test(indicator-test
    indicator-test
)

if (ENABLE_LOMIRI_FEATURES)
add_subdirectory(integration)
endif()
add_subdirectory(dbus-types)
add_subdirectory(service-mocks)

# coverage reporting
set(COVERAGE_TEST_TARGETS
  ${COVERAGE_TEST_TARGETS}
  name-watch-test
  accounts-service-user-test
  volume-control-test
  sound-menu-test
  notifications-test
  media-player-user-test
  greeter-list-test
  indicator-test
  PARENT_SCOPE
)
