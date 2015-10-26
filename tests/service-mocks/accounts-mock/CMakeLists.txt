set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5DBus REQUIRED)
include_directories(${Qt5DBus_INCLUDE_DIRS}
                    "${CMAKE_SOURCE_DIR}/tests/service-mocks")

add_definitions(-DQT_NO_KEYWORDS=1)

set(dbusinterface_sound_xml "com.ubuntu.AccountsService.Sound.Mock.xml")
set_source_files_properties(${dbusinterface_sound_xml} PROPERTIES
  CLASSNAME AccountsServiceSoundMockInterface)

qt5_add_dbus_interface(interface_files ${dbusinterface_sound_xml} accountsservice_sound_interface)

qt5_add_dbus_adaptor(adaptor_files
    com.ubuntu.AccountsService.Sound.Mock.xml
    AccountsServiceSoundMock.h
    ubuntu::indicators::testing::AccountsServiceSoundMock
    AccountsServiceSoundMockAdaptor)

qt5_add_dbus_adaptor(adaptor_files
    ${CMAKE_SOURCE_DIR}/tests/dbus-types/org.freedesktop.Accounts.xml
    AccountsMock.h
    ubuntu::indicators::testing::AccountsMock
    AccountsMockAdaptor)

add_executable(
  accounts-service-sound
  ${interface_files}
  ${adaptor_files}
  AccountsServiceSoundMock.cpp
  AccountsMock.cpp
  ${CMAKE_SOURCE_DIR}/tests/service-mocks/DBusPropertiesNotifier.cpp
  main.cpp
)

qt5_use_modules(
    accounts-service-sound
    Core
    DBus
)
