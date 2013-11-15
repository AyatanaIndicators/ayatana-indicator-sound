
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

add_executable (name-watch-test name-watch-test.cc)
target_link_libraries (name-watch-test gtest ${GIO2_LIBRARIES})
add_test(name-watch-test name-watch-test)

