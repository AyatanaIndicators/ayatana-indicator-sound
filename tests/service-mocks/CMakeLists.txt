if(EXISTS "/usr/share/accountsservice/interfaces/com.lomiri.AccountsService.Sound.xml")
    add_subdirectory(accounts-mock)
endif()
add_subdirectory(media-player-mpris-mock)
