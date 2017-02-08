pkg_check_modules(UNITY_API libunity-api>=0.1.3 REQUIRED)
include_directories(${UNITY_API_INCLUDE_DIRS})

include_directories("${CMAKE_SOURCE_DIR}/include")

add_library(
    gmenuharness-shared SHARED
    MatchResult.cpp
    MatchUtils.cpp
    MenuItemMatcher.cpp
    MenuMatcher.cpp
)

target_link_libraries(
    gmenuharness-shared
    ${GLIB_LDFLAGS}
)

# coverage reporting
set(COVERAGE_TARGETS
  ${COVERAGE_TARGETS}
  gmenuharness-shared
  PARENT_SCOPE
)
