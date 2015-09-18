set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5DBus REQUIRED)
include_directories(${Qt5DBus_INCLUDE_DIRS})

add_definitions(-DQT_NO_KEYWORDS=1)

set(dbusinterface_sound_xml "com.ubuntu.AccountsService.Sound.Mock.xml")
set_source_files_properties(${dbusinterface_sound_xml} PROPERTIES
  CLASSNAME AccountsServiceSoundMockInterface)

qt5_add_dbus_interface(interface_files ${dbusinterface_sound_xml} accountsservice_sound_interface)

qt5_add_dbus_adaptor(adaptor_files
    com.ubuntu.AccountsService.Sound.Mock.xml
    AccountsServiceSoundMock.h
    AccountsServiceSoundMock
    AccountsServiceSoundMockAdaptor)

qt5_add_dbus_adaptor(adaptor_files
    org.freedesktop.Accounts.Mock.xml
    AccountsMock.h
    AccountsMock
    AccountsMockAdaptor)

add_executable(
  accounts-service-sound
  ${interface_files}
  ${adaptor_files}
  AccountsServiceSoundMock.cpp
  AccountsMock.cpp
  main.cpp
)

qt5_use_modules(
    accounts-service-sound
    Core
    DBus
)
