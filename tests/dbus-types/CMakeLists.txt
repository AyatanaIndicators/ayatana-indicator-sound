set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5DBus REQUIRED)
include_directories(${Qt5DBus_INCLUDE_DIRS})

add_definitions(-DQT_NO_KEYWORDS=1)

set(dbusinterface_streamrestore_xml "org.PulseAudio.Ext.StreamRestore1.xml")
set_source_files_properties(${dbusinterface_streamrestore_xml} PROPERTIES
  CLASSNAME StreamRestoreInterface)

set(dbusinterface_accounts_xml "org.freedesktop.Accounts.xml")
set_source_files_properties(${dbusinterface_accounts_xml} PROPERTIES
  CLASSNAME AccountsInterface)

set(dbusinterface_accountssound_xml "com.ubuntu.AccountsService.Sound.xml")
set_source_files_properties(${dbusinterface_accountssound_xml} PROPERTIES
  CLASSNAME AccountsSoundInterface)

set(dbusinterface_properties_xml "org.freedesktop.DBus.Properties.xml")
set_source_files_properties(${dbusinterface_properties_xml} PROPERTIES
  CLASSNAME DBusPropertiesInterface
  NO_NAMESPACE YES
  INCLUDE "dbus-types.h")

set(dbusinterface_menus_xml "org.gtk.Menus.xml")
set_source_files_properties(${dbusinterface_menus_xml} PROPERTIES
  CLASSNAME MenusInterface)

qt5_add_dbus_interface(interface_files ${dbusinterface_streamrestore_xml} stream_restore_interface)
qt5_add_dbus_interface(interface_files ${dbusinterface_properties_xml} dbus_properties_interface)
qt5_add_dbus_interface(interface_files ${dbusinterface_accounts_xml} dbus_accounts_interface)
qt5_add_dbus_interface(interface_files ${dbusinterface_accountssound_xml} dbus_accountssound_interface)
qt5_add_dbus_interface(interface_files ${dbusinterface_menus_xml} dbus_menus_interface)

add_library(
  sound-indicator-dbus-interfaces
  STATIC
  ${interface_files}
  pulseaudio-volume.cpp
)

qt5_use_modules(
    sound-indicator-dbus-interfaces
    Core
    DBus
)
