cmake_minimum_required(VERSION 3.22)
project(proxy C)

set(CMAKE_C_STANDARD 11)
set(COMPILE_FLAGS "-lpthread -lcurl")


find_package(CURL REQUIRED)
add_executable(proxy proxy.c)
include_directories(${CURL_INCLUDE_DIR})
target_link_libraries(proxy ${CURL_LIBRARIES})
target_include_directories(proxy PUBLIC include)
