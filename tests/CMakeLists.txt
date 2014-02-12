
###########################
# Google Test
###########################

include_directories(${GTEST_INCLUDE_DIR})

add_library (gtest STATIC
             ${GTEST_SOURCE_DIR}/gtest-all.cc
             ${GTEST_SOURCE_DIR}/gtest_main.cc)
target_link_libraries(gtest ${GTEST_LIBS})


###########################
# Name Watch Test
###########################

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable (name-watch-test name-watch-test.cc ${CMAKE_SOURCE_DIR}/src/bus-watch-namespace.c)
target_link_libraries (name-watch-test gtest ${SOUNDSERVICE_LIBRARIES})
add_test(name-watch-test name-watch-test)

###########################
# Accounts Service User
###########################

include_directories(${CMAKE_SOURCE_DIR}/src)
add_executable (accounts-service-user-test accounts-service-user.cc)
target_link_libraries (
	accounts-service-user-test
	indicator-sound-service-lib
	gtest
	${SOUNDSERVICE_LIBRARIES}
	${TEST_LIBRARIES}
)
add_test(accounts-service-user-test accounts-service-user-test)

