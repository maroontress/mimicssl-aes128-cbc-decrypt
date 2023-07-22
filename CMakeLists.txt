cmake_minimum_required(VERSION 3.24)

project("mimicssl-aes128-cbc" VERSION 1.0.0)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_subdirectory(libmimicssl-aes128-cbc-decrypt)
add_subdirectory(evp-example-cli)

option(BUILD_TESTSUITE "Build testsuite" ON)
if (${BUILD_TESTSUITE})
  include(CTest)
  add_subdirectory(testsuite)
endif()
