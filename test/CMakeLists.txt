cmake_minimum_required(VERSION 3.14)

set(UNIT_TEST_BINARY flux_tests)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

set(UNIT_TEST_FILES
  transducers.test.cpp
)

add_executable(${UNIT_TEST_BINARY}
  ${UNIT_TEST_FILES}
)

target_link_libraries(${UNIT_TEST_BINARY}
  gtest
  gtest_main
  gmock
  gmock_main
)

gtest_discover_tests(${UNIT_TEST_BINARY})
