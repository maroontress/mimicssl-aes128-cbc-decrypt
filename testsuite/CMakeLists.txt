set(CMAKE_CXX_STANDARD 23)

file(COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/alice.md.decrypted
    ${CMAKE_CURRENT_SOURCE_DIR}/alice.md.encrypted
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

enable_testing()

add_executable(testsuite)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
    if("${CMAKE_ANDROID_ARCH_ABI}" STREQUAL "arm64-v8a")
        set(SOURCES arm_v7_main.cxx)
    elseif("${CMAKE_ANDROID_ARCH_ABI}" STREQUAL "armeabi-v7a")
        set(OPTIONS -mfpu=neon)
        set(SOURCES arm_v7_main.cxx)
    elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
        set(OPTIONS -msse3 -maes)
        set(SOURCES x86_64_main.cxx)
    else()
        set(SOURCES main.cxx)
    endif()
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin"
        AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
    set(SOURCES aarch64_main.cxx)
elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
    set(OPTIONS -msse3 -maes)
    set(SOURCES x86_64_main.cxx)
else()
    set(SOURCES main.cxx)
endif()

target_compile_options(testsuite PUBLIC ${OPTIONS})
target_sources(testsuite PUBLIC
    ${SOURCES}
    expect.hxx expect_fallback.hxx)
target_include_directories(testsuite PRIVATE
    mimicssl-aes128-cbc-decrypt
    ${CMAKE_SOURCE_DIR}/libmimicssl-aes128-cbc-decrypt/src
    ${CMAKE_SOURCE_DIR}/lighter/include)

target_link_libraries(testsuite mimicssl-aes128-cbc-decrypt)

include(GoogleTest)
gtest_discover_tests(testsuite DISCOVERY_TIMEOUT 50)
