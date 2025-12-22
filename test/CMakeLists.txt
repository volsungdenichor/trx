add_executable(flux_test
  flux.test.cpp
)

target_link_libraries(flux_test
  gtest_main
  gtest
)

include(GoogleTest)
gtest_discover_tests(flux_test)
