set(CMAKE_C_STANDARD 23)

if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"
        OR "${CMAKE_C_COMPILER_ID}" STREQUAL "AppleClang"
        OR "${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -Wpedantic)
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_C_FLAGS_RELEASE "-O3")
elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    add_compile_options(/W4 /WX)
endif()

add_library(mimicssl-aes128-cbc-decrypt STATIC)
add_library(mimicssl-aes128-cbc-decrypt-shared SHARED)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
    if("${CMAKE_ANDROID_ARCH_ABI}" STREQUAL "arm64-v8a")
        set(SOURCES src/evp.c src/arm_v7_Aes128Cbc.c)
    elseif("${CMAKE_ANDROID_ARCH_ABI}" STREQUAL "armeabi-v7a")
        set(OPTIONS -mfpu=neon)
        set(SOURCES src/evp.c src/arm_v7_Aes128Cbc.c)
    elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
        set(OPTIONS -msse3 -maes)
        set(SOURCES src/evp.c src/x86_64_Aes128Cbc.c)
    else()
        set(SOURCES src/evp.c src/Aes128Cbc.c)
    endif()
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS")
    if("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "arm64")
        set(SOURCES src/evp.c src/aarch64_Aes128Cbc.c)
    elseif("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "x86_64")
        set(OPTIONS -msse3 -maes)
        set(SOURCES src/evp.c src/x86_64_Aes128Cbc.c)
    endif()
    set_target_properties(mimicssl-aes128-cbc-decrypt-shared PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
        MACOSX_BUNDLE_BUNDLE_NAME mimicssl-aes128-cbc-decrypt
        MACOSX_BUNDLE_BUNDLE_VERSION 1.0
        MACOSX_BUNDLE_SHORT_VERSION_STRING 1.0
        MACOSX_BUNDLE_LONG_VERSION_STRING 1.0)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin"
        AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
    set(SOURCES src/evp.c src/aarch64_Aes128Cbc.c)
elseif("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
    set(OPTIONS -msse3 -maes)
    set(SOURCES src/evp.c src/x86_64_Aes128Cbc.c)
else()
    set(SOURCES src/evp.c src/Aes128Cbc.c)
endif()

include(GenerateExportHeader)

generate_export_header(mimicssl-aes128-cbc-decrypt
    BASE_NAME EVP
    EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/evp_export.h)
target_compile_options(mimicssl-aes128-cbc-decrypt PUBLIC ${OPTIONS})
target_sources(mimicssl-aes128-cbc-decrypt PRIVATE ${SOURCES})
target_include_directories(mimicssl-aes128-cbc-decrypt PUBLIC
    include
    ${PROJECT_BINARY_DIR})

target_compile_options(mimicssl-aes128-cbc-decrypt-shared PUBLIC ${OPTIONS})
target_sources(mimicssl-aes128-cbc-decrypt-shared PRIVATE ${SOURCES})
target_include_directories(mimicssl-aes128-cbc-decrypt-shared PUBLIC
    include
    ${PROJECT_BINARY_DIR})
set_target_properties(mimicssl-aes128-cbc-decrypt-shared
    PROPERTIES OUTPUT_NAME mimicssl-aes128-cbc-decrypt)

include(GNUInstallDirs)
install(TARGETS mimicssl-aes128-cbc-decrypt DESTINATION lib)
install(TARGETS mimicssl-aes128-cbc-decrypt-shared DESTINATION lib)
install(FILES
    include/evp.h
    ${PROJECT_BINARY_DIR}/evp_export.h
    DESTINATION include/mimicssl)
