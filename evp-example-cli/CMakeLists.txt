set(CMAKE_CXX_STANDARD 23)

add_executable(evp-example-cli main.cxx)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS")
    set_target_properties(evp-example-cli PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        XCODE_ATTRIBUTE_ENABLE_BITCODE "NO"
        MACOSX_BUNDLE_BUNDLE_NAME evp-example-cli
        MACOSX_BUNDLE_BUNDLE_VERSION 1.0
        MACOSX_BUNDLE_SHORT_VERSION_STRING 1.0
        MACOSX_BUNDLE_LONG_VERSION_STRING 1.0)
endif()

target_include_directories(evp-example-cli PRIVATE
    mimicssl-aes128-cbc-decrypt)

target_link_libraries(evp-example-cli mimicssl-aes128-cbc-decrypt)
