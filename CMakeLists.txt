cmake_minimum_required(VERSION 3.27)

project("mimicssl-aes128-cbc" VERSION 1.0.2)

cmake_policy(SET CMP0142 NEW)

include(CTest)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

add_subdirectory(libmimicssl-aes128-cbc-decrypt)
add_subdirectory(evp-example-cli)
add_subdirectory(testsuite)
