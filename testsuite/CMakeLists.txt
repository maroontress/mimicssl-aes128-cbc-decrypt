set(CMAKE_CXX_STANDARD 23)

file(COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/alice.md.decrypted
    ${CMAKE_CURRENT_SOURCE_DIR}/alice.md.encrypted
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

enable_testing()

add_executable(testsuite
    main.cxx expect.hxx expect_fallback.hxx)

target_include_directories(testsuite PRIVATE
    mimicssl-aes128-cbc-decrypt
    ${CMAKE_SOURCE_DIR}/libmimicssl-aes128-cbc-decrypt/src
    ${CMAKE_SOURCE_DIR}/lighter/include)

target_link_libraries(testsuite mimicssl-aes128-cbc-decrypt)

include(GoogleTest)
gtest_discover_tests(testsuite)
