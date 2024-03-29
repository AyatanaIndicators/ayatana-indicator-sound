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

if(ENABLE_LOMIRI_FEATURES)
    set_source_files_properties("com.lomiri.AccountsService.Sound.xml" PROPERTIES
      CLASSNAME AccountsSoundInterface)
endif()

set(dbusinterface_properties_xml "org.freedesktop.DBus.Properties.xml")
set_source_files_properties(${dbusinterface_properties_xml} PROPERTIES
  CLASSNAME DBusPropertiesInterface
  NO_NAMESPACE YES
  INCLUDE "dbus-types.h")

set(dbusinterface_actions_xml "org.gtk.Actions.xml")
set_source_files_properties(${dbusinterface_actions_xml} PROPERTIES
  CLASSNAME MenusInterface
  INCLUDE "dbus-action-result.h")

set(dbusinterface_notifications_xml "org.freedesktop.Notifications.xml")
set_source_files_properties(${dbusinterface_notifications_xml} PROPERTIES
  CLASSNAME NotificationsInterface)

qt5_add_dbus_interface(interface_files ${dbusinterface_streamrestore_xml} stream_restore_interface)
qt5_add_dbus_interface(interface_files ${dbusinterface_properties_xml} dbus_properties_interface)
qt5_add_dbus_interface(interface_files ${dbusinterface_accounts_xml} dbus_accounts_interface)

if(ENABLE_LOMIRI_FEATURES)
    qt5_add_dbus_interface(interface_files "com.lomiri.AccountsService.Sound.xml" dbus_accountssound_interface)
endif()

qt5_add_dbus_interface(interface_files ${dbusinterface_actions_xml} dbus_menus_interface)
qt5_add_dbus_interface(interface_files ${dbusinterface_notifications_xml} dbus_notifications_interface)

add_library(
  sound-indicator-dbus-interfaces
  STATIC
  ${interface_files}
  pulseaudio-volume.cpp
  dbus-action-result.cpp
)

target_link_libraries(
    sound-indicator-dbus-interfaces
    Qt5::Core
    Qt5::DBus
)
