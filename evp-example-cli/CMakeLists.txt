set(CMAKE_CXX_STANDARD 23)

add_executable(evp-example-cli main.cxx)

target_include_directories(evp-example-cli PRIVATE
    mimicssl-aes128-cbc-decrypt)

target_link_libraries(evp-example-cli mimicssl-aes128-cbc-decrypt)
