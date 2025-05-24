cmake_minimum_required(VERSION 3.14)
project(stl2glb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/cpp-httplib
        ${CMAKE_CURRENT_SOURCE_DIR}/external/tinygltf
        ${CMAKE_CURRENT_SOURCE_DIR}/external/minio-cpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/json/include
)

file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(stl2glb_server ${SOURCES})

target_link_libraries(stl2glb_server
        PRIVATE
        ssl
        crypto
        curl
        curlpp
)
